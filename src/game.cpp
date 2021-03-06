#include "game.hpp"

//Define instance and id counter
Game* Game::mInstance = nullptr;
unsigned int Game::mUniqueId = 0;

Game::Game()
	: mMvp{ glm::mat4{1.f} }, mLastFrameTime{ -1 }, mLastSyncedPlayer{ 0 }
{
	for (const std::string& shaderName : allShaderNames)
		loadShader(shaderName);
}

void Game::detectCollisions()
{
	ZoneScoped;
	if (mPlayers.size() > 0 && mCollectPool.getNumEnabled() > 0)
	{
		for (size_t i = 0; i < mPlayers.size(); i++)
		{
			glm::quat playerQuat = mPlayers[i].getPosition();
			for (size_t j = 0; j < mCollectPool.getNumEnabled(); j++)
			{
				glm::quat collectibleQuat = mCollectPool[j].getPosition();
				glm::quat deltaQuat = glm::normalize(glm::inverse(playerQuat) * collectibleQuat);

				//Collision detection by comparing how small the angle between the objects are
				//From https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
				auto sinxPart = 2.f * (deltaQuat.w * deltaQuat.x + deltaQuat.y * deltaQuat.z);
				auto cosxPart = 1.f - 2.f * (deltaQuat.x * deltaQuat.x + deltaQuat.y * deltaQuat.y);
				auto xAngle = std::atan2(sinxPart, cosxPart);

				auto sinyPart = 2.f * (deltaQuat.w * deltaQuat.y - deltaQuat.z * deltaQuat.x);
				auto yAngle = std::asin(sinyPart);

				if (std::abs(xAngle) <= collisionDistance && std::abs(yAngle) <= collisionDistance)
				{
					mPlayers[i].addPoints();
                    mCollectPool.disableCollectibleAndSwap(j);
                    mIdPoints.push_back(std::make_pair(i, mPlayers[i].getPoints()));
				}
			}			
		}
	}
}

void Game::spawnCollectibles(float currentFrameTime)
{
	if ((int)currentFrameTime % mPosGenerator.spawnTime == 0 && !mPosGenerator.hasSpawnedThisInterval)
	{
		for (size_t i = 0; i < mPlayers.size(); i++)
		{
			mCollectPool.enableCollectible(mPosGenerator.generatePos());
		}
		mPosGenerator.hasSpawnedThisInterval = true;
	}

	if ((int)currentFrameTime % mPosGenerator.spawnTime == 1)
		mPosGenerator.hasSpawnedThisInterval = false;
}

void Game::init()
{
	mInstance = new Game{};
	mInstance->mIdPoints.reserve(mMAXPLAYERS);
	mInstance->printLoadedAssets();
	mInstance->mCollectPool.init();
	mInstance->mPlayers.reserve(mMAXPLAYERS);	
	mInstance->setBackground(new BackgroundObject());
	mInstance->mPosGenerator.init();
}

Game& Game::instance()
{
	if (!mInstance) {
		mInstance = new Game{};
	}
	return *mInstance;
}

void Game::destroy()
{
	if (mInstance)
	{
		delete mInstance->mBackground;
		delete mInstance;
	}
}

void Game::printShaderPrograms() const
{
	std::string output = "Loaded shaders:";
	for (const std::string& s : mShaderNames)
	{
		output +="\n       " + s;
	}
	sgct::Log::Info(output.c_str());
}

void Game::printLoadedAssets() const
{
	mInstance->printShaderPrograms();
}

void Game::render() const
{
	ZoneScoped;
	//Render background
	mBackground->render(mMvp, mV);

	glClear(GL_DEPTH_BUFFER_BIT); //Draw all other objects in front of background

	renderPlayers();

	mCollectPool.render(mMvp, mV);
}

void Game::addPlayer()
{
	mPlayers.emplace_back();
	++mUniqueId;
}

void Game::addCollectible()
{
	mCollectPool.enableCollectible(mPosGenerator.generatePos());
}

void Game::addPlayer(const glm::vec3& pos)
{
	mPlayers.push_back(Player{ "diver", DOMERADIUS, pos, 0.f, "Player " + std::to_string(mUniqueId), 0.5 });
	++mUniqueId;
}

void Game::addPlayer(const PlayerData& newPlayerData, const PositionData& newPosData)
{
	//Create player from PositionData object
	mPlayers.emplace_back(newPlayerData, newPosData);
}

void Game::addPlayer(std::tuple<unsigned int, std::string>&& inputTuple)
{
	assert(std::get<0>(inputTuple) == mPlayers.size() && "Player creation desync (id out of bounds: mPlayers)");
	mPlayers.emplace_back(std::get<1>(inputTuple), mPosGenerator.generatePos());
}

void Game::update()
{
	if (mGameIsStarted) {

		ZoneScoped;
		if (mGameIsEnded)
			return;
		if (mLastFrameTime == -1) //First update?
		{
			mLastFrameTime = static_cast<float>(sgct::Engine::getTime());
			return;
		}

		float currentFrameTime = static_cast<float>(sgct::Engine::getTime());

		float deltaTime = currentFrameTime - mLastFrameTime;
		this->mTotalTime += deltaTime;
		if (mTotalTime > mMaxTime && mGameIsStarted) {
			this->endGame();
		}

		spawnCollectibles(currentFrameTime);

		//Update players
		for (auto& player : mPlayers)
			player.update(deltaTime);

		//for (size_t i = 0; i < CollectiblePool::mMAXNUMCOLLECTIBLES && mCollectPool[i].isEnabled(); i++)
		for (size_t i = 0; i < CollectiblePool::mMAXNUMCOLLECTIBLES; i++)
		{
			mCollectPool[i].update(deltaTime);
		}

		//TODO Update other type of objects

		detectCollisions();

		mLastFrameTime = currentFrameTime;
	}

}

std::string Game::getLeaderboard() const
{
	//Alias for pair of player name and points
	using pointPair = std::pair<std::string, int>;

	std::stringstream output;

	std::vector<pointPair> sortedPlayersAndPoints;
	sortedPlayersAndPoints.reserve(mPlayers.size());

	//Make pairs of each players name and points
	for (const auto& player : mPlayers)
	{
		sortedPlayersAndPoints.push_back(std::make_pair(player.getName(), player.getPoints()));
	}

	//Sort decreasingly
	std::sort(sortedPlayersAndPoints.begin(), sortedPlayersAndPoints.end(),
		[](const pointPair& a, const pointPair& b)
		{
			return a.second > b.second;
		});

	for (size_t i = 0; i < sortedPlayersAndPoints.size(); i++)
	{
		output << std::setw(20) << std::left << sortedPlayersAndPoints[i].first;
		output << " - " << std::setw(8) << std::right << sortedPlayersAndPoints[i].second;
		output << "\n";

		if (i >= 10)
			break;
	}

	return output.str();
}

void Game::sendPointsToServer(std::unique_ptr<WebSocketHandler>& ws)
{
	//Iterate over mIdPoints to get id's and new points
	//Send these to server through ws
    for (size_t i = 0; i < mIdPoints.size(); i++)
    {
        std::string playerId = std::to_string(mIdPoints[i].first);
        std::string points = std::to_string(mIdPoints[i].second);
        ws->queueMessage("P " + playerId + "   " + points);
    }
	mIdPoints.clear();
}

std::vector<SyncableData> Game::getSyncableData()
{
	std::vector<SyncableData> tempData;
	tempData.reserve(mCollectPool.getNumEnabled() * 1.5);

	//Sync data for players present on all nodes first
	for (size_t i = 0; i < mLastSyncedPlayer; ++i)
	{
		SyncableData tempState;
		Player& currentPlayer = mPlayers[i];

		tempState.mPlayerData = currentPlayer.getPlayerData(false);
		tempState.mPositionData = currentPlayer.getPositionData();
		tempState.mIsPlayer = true;

		tempData.push_back(tempState);
	}

	//Players not present on client nodes needs some book keeping
	for (size_t i = mLastSyncedPlayer; i < mPlayers.size(); ++i)
	{
		SyncableData tempState;
		Player& currentPlayer = mPlayers[i];

		tempState.mPlayerData = currentPlayer.getPlayerData(true);
		tempState.mPositionData = currentPlayer.getPositionData();
		tempState.mIsPlayer = true;

		tempData.push_back(tempState);
	}

	//Get enabled collectibles
	for (size_t i = 0; i < mCollectPool.getNumEnabled(); i++)
	{
		SyncableData tempState;
		Collectible& currentCollectible = mCollectPool[i];

		tempState.mCollectData = currentCollectible.getCollectibleData(i);
		tempState.mPositionData = currentCollectible.getPositionData();
		tempState.mIsPlayer = false;

		tempData.push_back(tempState);
	}

	return tempData;
}

void Game::setSyncableData(const std::vector<SyncableData> newState) //Copy atm bc of sgct synchronocity issues
{
	if (newState.size() > 10000) //If this happens something got corrupt along the way
		return;

	std::vector<SyncableData> newPlayerStates;
	newPlayerStates.reserve(newState.size() / 2 + 1);
  
	std::vector<SyncableData> newCollectibleStates;
	newCollectibleStates.reserve(newState.size() / 1.5 + 1);

	size_t i = 0;
	while (i < newState.size())
	{
		if (newState[i].mIsPlayer)
			newPlayerStates.push_back(newState[i++]);
		else
			newCollectibleStates.push_back(newState[i++]);
	}

	if (newPlayerStates.size() > 0)
		setDecodedPlayerData(newPlayerStates);
	if (newCollectibleStates.size() > 0)
		setDecodedCollectibleData(newCollectibleStates);	
}

void Game::startGame()
{
	this->mTotalTime = 0;
	this->mGameIsStarted = true;
}

float Game::getPassedTime()
{
	float var = this->mTotalTime/this->mMaxTime;
	float value = (int)(var * 100 + .5);
	return (float)value / 100;
}

bool Game::shouldSendTime()
{
	if (mTotalTime - mLastTime > 1) {
		mLastTime = mTotalTime;
		return true;
		
	}
	return false;
}

void Game::setDecodedCollectibleData(const std::vector<SyncableData>& newState)
{
	mCollectPool.setNumEnabled(newState.size());
	for (size_t i = 0; i < newState.size(); i++)
	{
		const SyncableData& currentState = newState[i];
		mCollectPool[i].setCollectibleData(currentState.mPositionData, currentState.mCollectData.mModelIndex);
	}

	//No need to disable any unactive elements as nodes only render
}

void Game::renderPlayers() const
{
	ZoneScoped;
	if (mPlayers.size() > 0)
	{
		auto const& playerShader = sgct::ShaderManager::instance().shaderProgram("player");
		playerShader.bind();

		for (const Player& p : mPlayers)
			p.render(mMvp, mV);

		playerShader.unbind();
	}
}

void Game::setDecodedPlayerData(const std::vector<SyncableData>& newState)
{
	size_t nUnsyncedPlayers = mPlayers.size();

	//New players get instanciated with correct value from the start
	if (newState.size() > mPlayers.size())
	{
		for (size_t i = nUnsyncedPlayers; i < newState.size(); ++i)
		{
			addPlayer(newState[i].mPlayerData, newState[i].mPositionData);
		}
	}

	for (size_t i = 0; i < nUnsyncedPlayers; ++i)
	{
		mPlayers[i].setPlayerData(newState[i].mPlayerData, newState[i].mPositionData);
	}
	nUnsyncedPlayers = mPlayers.size();	
}

void Game::updateTurnSpeed(std::tuple<unsigned int, float>&& input)
{
	unsigned id = std::get<0>(input);
	float rotAngle = std::get<1>(input);

	assert(id < mPlayers.size() && "Player update turn speed desync (id out of bounds mPlayers");

	mPlayers[id].setTurnSpeed(rotAngle);
}

void Game::enablePlayer(unsigned id)
{
	assert(id < mPlayers.size() && "Player disable desync (id out of bounds mPlayers");
	mPlayers[id].enablePlayer();
}

void Game::disablePlayer(unsigned id)
{
	assert(id < mPlayers.size() && "Player disable desync (id out of bounds mPlayers");
	mPlayers[id].disablePlayer();
}

void Game::rotateAllPlayers(float newOrientation)
{
	for (auto& player : mPlayers)
	{
		player.setOrientation(player.getOrientation() + newOrientation);
	}
}

std::pair<glm::vec3, glm::vec3> Game::getPlayerColours(unsigned id)
{
    assert(id < mPlayers.size() && "Player get colours desync (id out of bounds mPlayers");
    return mPlayers[id].getColours();
}

void Game::loadShader(const std::string& shaderName)
{
	//Define path and strings to hold shaders
	std::string path = Utility::findRootDir() + "/src/shaders/" + shaderName;
	std::string vert, frag;

	//Open streams to shader files
	std::ifstream in_vert{ path + "vert.glsl" };
	std::ifstream in_frag{ path + "frag.glsl" };
	
	//Read shaders into strings
	if (in_vert.good() && in_frag.good()) {
		vert = std::string(std::istreambuf_iterator<char>(in_vert), {});
		frag = std::string(std::istreambuf_iterator<char>(in_frag), {});
	}
	else
	{
		sgct::Log::Error("ERROR OPENING SHADER FILE: %s", shaderName.c_str());
	}
	in_vert.close(); in_frag.close();

	mShaderNames.push_back(shaderName);
	sgct::ShaderManager::instance().addShaderProgram(shaderName, vert, frag);
}
