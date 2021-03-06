#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>
#include <tuple>
#include <cmath>
#include <random>
#include <cstddef>

#include "sgct/shareddata.h"
#include "sgct/log.h"
#include "sgct/profiling.h"
#include "sgct/shadermanager.h"
#include "sgct/shaderprogram.h"
#include "sgct/engine.h"
#include "glad/glad.h"
#include "glm/packing.hpp"
#include "glm/matrix.hpp"

#include "player.hpp"
#include "collectiblepool.hpp"
#include "utility.hpp"
#include "backgroundobject.hpp"
#include "websockethandler.h"

//Because sgct can't handle syncting separate vectors all sync data gets put in one vector
//This needs a master type to handle all syncable objects
struct SyncableData
{
	PlayerData mPlayerData;
	PositionData mPositionData;
	CollectibleData mCollectData;
	bool mIsPlayer;
};

//Implemented as explicit singleton, handles pretty much everything
class Game
{
public:
	//Init instance and print useful shader and model info
	static void init();

	//Get instance
	static Game& instance();
	static bool exists() { return mInstance != nullptr; }

	//Destroy instance
	static void destroy();

	//Copying forbidden
	Game(Game const&) = delete;
	void operator=(Game const&) = delete;

	//Print loaded assets (shaders, models)
	void printLoadedAssets() const;

	//Render objects
	void render() const;

	//Set MVP matrix
	void setMVP(const glm::mat4& mvp) { mMvp = mvp;};

	//Set view matrix
	void setV(const glm::mat4& v) { mV = v; }

	//Used for debugging
	void addPlayer();
	void addCollectible();

	void addPlayer(const glm::vec3& pos);

	//Add player from playerdata for instant sync
	void addPlayer(const PlayerData& newPlayerData,
				   const PositionData& newPosData);

	//Add player from server request
	void addPlayer(std::tuple<unsigned int, std::string>&& inputTuple);

	//enable/disable player
	void enablePlayer(unsigned id);
	void disablePlayer(unsigned id);

	//Update all gameobjects
	void update();

	//Get leaderboard string
	//Only gets called at end of game
	std::string getLeaderboard() const;

	//Check if game has ended
	bool hasGameEnded() const { return mGameIsEnded; }

	//End the game (stop updating state)
	void endGame() { mGameIsEnded = true; }

	//Set game time
	void setMaxTime(float time) { mMaxTime = time; }

	//Update point data on phone
	void sendPointsToServer(std::unique_ptr<WebSocketHandler>& ws);

	//Set the turn speed of player player with id id
	void updateTurnSpeed(std::tuple<unsigned int, float>&& input);

	//DEBUGGING TOOL: apply orientation to all GameObjects
	void rotateAllPlayers(float deltaOrientation);

    //Get and return player-colours
    std::pair<glm::vec3, glm::vec3> getPlayerColours(unsigned id);

	static constexpr size_t mMAXPLAYERS = 110;
	static constexpr size_t mMAXCOLLECTIBLES = 300;

	std::vector<SyncableData> getSyncableData();
	void setSyncableData(const std::vector<SyncableData> newState);

	//start timer
	void startGame();
	float getPassedTime();
	bool shouldSendTime();

private:
//Members
	//Singleton instance of game
	static Game* mInstance;

	//All players stored sequentually
	std::vector<Player> mPlayers;

	//Pool of collectibles for fast "generation" of objects
	CollectiblePool mCollectPool;

	//Has the game ended?
	bool mGameIsEnded = false;

	//GameObjects unique id generator for player tagging
	//Deprecated
	static unsigned int mUniqueId;

	//Track all loaded shaders' names
	std::vector<std::string> mShaderNames;

	//Container to store player id and new points
	//Data sent to server to update score on each player's phone
	std::vector<std::pair<unsigned, int>> mIdPoints;

	//MVP matrix used for rendering
	glm::mat4 mMvp;

	//View matrix
	glm::mat4 mV;

	//Slot after which players only present on master node exist
	size_t mLastSyncedPlayer;

	//The time of the last update (in seconds)
	float mLastFrameTime;

	static constexpr double collisionDistance = 0.1f; //TODO make this object specific

	BackgroundObject *mBackground; //Holds pointer to the background

	float mTotalTime = 0, mMaxTime = 60;//seconds
	float mLastTime = 0;
	bool mGameIsStarted = false;

//Functions
	//Constructor
	Game();

	//Collision detection in mInteractObjects, bubble style
	void detectCollisions();

	//Spawn Collectibles
	void spawnCollectibles(float currentFrameTime);

	//Set object data from inputted data
	void setDecodedPlayerData(const std::vector<SyncableData>& newState);
	void setDecodedCollectibleData(const std::vector<SyncableData>& newState);

	void renderPlayers() const;

	//Read shader into ShaderManager
	void loadShader(const std::string& shaderName);

	//Set background
	void setBackground(BackgroundObject* background){
		mBackground = background;
	}

	//Display current list of shaders, called by printLoadedAssets()
	void printShaderPrograms() const;

	const glm::mat4& getMVP() { return mMvp; };
	const glm::mat4& getV() { return mV; };

	struct PositionGenerator
	{
		void init()
		{
			std::random_device randomDevice;
			gen = std::mt19937(randomDevice());
			rng = std::uniform_real_distribution<>(-1.5f, 1.5f);
		}

		//RNG stuff
		std::mt19937 gen;
		std::uniform_real_distribution<> rng;

		//State stuff
		bool hasSpawnedThisInterval = false;
		unsigned spawnTime = 4;

		glm::vec3 generatePos()
		{
			ZoneScoped;
			return glm::vec3(1.5f + rng(gen), rng(gen), 0.f);
		}

	} mPosGenerator;
};
