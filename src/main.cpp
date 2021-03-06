//
//  Main.cpp provided under CC0 license
//
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <glm/gtx/string_cast.hpp>
#include "sgct/sgct.h"

#include "sgct/profiling.h"

#include "websockethandler.h"
#include "utility.hpp"
#include "game.hpp"
#include "modelmanager.hpp"
#include "inireader.h"

namespace {
	std::unique_ptr<WebSocketHandler> wsHandler;

	IniGroup spawnDetails;
	IniGroup gameConfig;

	bool bypassModelMatrix;

	//Variables to catch sync data
	bool isGameEnded = false, isGameStarted = false;
	bool areStatsVisible = false;

	//Container for deserialized game state info
	std::vector<SyncableData> gameObjectStates;
} // namespace

using namespace sgct;

/****************************
	FUNCTIONS DECLARATIONS
*****************************/
void initOGL(GLFWwindow*);
void draw(const RenderData& data);
void draw2D(const RenderData& data);
void cleanup();

std::vector<std::byte> encode();
void decode(const std::vector<std::byte>& data, unsigned int pos);
void preSync();
void postSyncPreDraw();

void keyboard(Key key, Modifier modifier, Action action, int);

void connectionEstablished();
void connectionClosed();
void messageReceived(const void* data, size_t length);

/****************************
		CONSTANTS
*****************************/
const std::string rootDir = Utility::findRootDir();

/****************************
			MAIN
*****************************/
int main(int argc, char** argv)
{
	std::vector<std::string> arg(argv + 1, argv + argc);
	Configuration config = sgct::parseArguments(arg);

	//Choose which config file (.xml) to open
	config.configFilename = rootDir + "/src/configs/fisheye_testing.xml";
	//config.configFilename = rootDir + "/src/configs/simple.xml";
	//config.configFilename = rootDir + "/src/configs/six_nodes.xml";
	//config.configFilename = rootDir + "/src/configs/two_fisheye_nodes.xml";

	config::Cluster cluster = sgct::loadCluster(config.configFilename);

	//Handle configs not directly related to sgct
	Ini appConfig;
	try
	{
		appConfig = readIni(rootDir + "/config.ini");
	}
	catch (const std::runtime_error & e)
	{
		Log::Error("%s", e.what());
		return EXIT_FAILURE;
	}
	IniGroup networkConfig = appConfig["Network"];
	IniGroup constraintConfig = appConfig["Constraint"];
		bypassModelMatrix = constraintConfig["bypassModelMatrix"] == "true";
		Player::setConstraints(std::stof(constraintConfig["fov"]),
		                       std::stof(constraintConfig["tilt"]));
	spawnDetails = appConfig["Spawn"];
	gameConfig = appConfig["Game"];

	//Provide functions to engine handles
	Engine::Callbacks callbacks;
	callbacks.initOpenGL = initOGL;
	callbacks.preSync = preSync;
	callbacks.encode = encode;
	callbacks.decode = decode;
	callbacks.postSyncPreDraw = postSyncPreDraw;
	callbacks.draw = draw;
	callbacks.draw2D = draw2D;
	callbacks.cleanup = cleanup;
	callbacks.keyboard = keyboard;

	//Initialize engine
	try {
		gameObjectStates.reserve(Game::mMAXPLAYERS*3);
		Engine::create(cluster, callbacks, config);
	}
	catch (const std::runtime_error & e) {
		Log::Error("%s", e.what());
		Engine::destroy();
		return EXIT_FAILURE;
	}

	if (Engine::instance().isMaster()) {
		wsHandler = std::make_unique<WebSocketHandler>(
			networkConfig["ip"],
			std::stoi(networkConfig["port"]),
			connectionEstablished,
			connectionClosed,
			messageReceived
		);
		constexpr const int MessageSize = 1024;
		wsHandler->connect("example-protocol", MessageSize);
	}
	/**********************************/
	/*			 Test Area			  */
	/**********************************/

	Engine::instance().render();

	Game::destroy();
	Engine::destroy();
	return EXIT_SUCCESS;
}

void initOGL(GLFWwindow*)
{
	ModelManager::init();
	Game::init();
	Game::instance().setMaxTime(std::stof(gameConfig["maxTime"]));

	/**********************************/
	/*			 Debug Area			  */
	/**********************************/
	if (Engine::instance().isMaster())
	{
		for (size_t i = 0; i < std::stoi(spawnDetails["numPlayers"]); i++)
		{
			Game::instance().addPlayer(glm::vec3(0.f + 0.3f * i));
		}
	}
}

void draw(const RenderData& data)
{	
	if (isGameStarted) {
		
		Game::instance().setMVP(data.modelViewProjectionMatrix);

		Game::instance().setV(data.viewMatrix);

		if (bypassModelMatrix)
			Game::instance().setMVP(data.projectionMatrix * data.viewMatrix);
		else
			Game::instance().setMVP(data.modelViewProjectionMatrix);

		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE); // TODO This should really be enabled but the normals of the
								  // background object are flipped atm
		glCullFace(GL_BACK);

		Game::instance().render();

		//GLenum err;
		//while ((err = glGetError()) != GL_NO_ERROR)
		//{
		//	sgct::Log::Error("GL Error: 0x%x", err);
		//}
	}
}

void draw2D(const RenderData& data)
{
	if (isGameStarted && !isGameEnded)
		return;

	static constexpr int bigFontSize = 20;
	static constexpr int smallFontSize = 14;

	const std::string leaderboardString = Game::instance().getLeaderboard();
	const glm::ivec2& screenRes = data.window.framebufferResolution();
	if (!isGameStarted) {
		text::print(
			data.window,
			data.viewport,
			*text::FontManager::instance().font("SGCTFont", bigFontSize),
			text::Alignment::TopCenter,
			screenRes.x / 2,
			screenRes.y / 2.5,
			glm::vec4{ 1.f, 0.5f, 0.f, 1.f },
			"%s", "Connect now"
		);
	}

	if (isGameEnded) {

	//Leaderboard header
	text::print(
		data.window,
		data.viewport,
		*text::FontManager::instance().font("SGCTFont", bigFontSize),
		text::Alignment::TopCenter,
		screenRes.x / 2,
		screenRes.y / 2,
		glm::vec4{ 1.f, 0.5f, 0.f, 1.f },
		"%s", "Leaderboard"
		);
	//Actual leaderboard
	text::print(
		data.window,
		data.viewport,
		*text::FontManager::instance().font("SGCTFont", smallFontSize),
		text::Alignment::TopCenter,
		screenRes.x / 2,
		screenRes.y / 2 - bigFontSize,
		glm::vec4{ 1.f, 0.5f, 0.f, 1.f },
		"%s", leaderboardString.c_str()
		);
	}
}

void keyboard(Key key, Modifier modifier, Action action, int)
{
	if (key == Key::Esc && action == Action::Press) {
		Engine::instance().terminate();
	}
	if (key == Key::Q && action == Action::Press) {
		Game::instance().endGame();
		isGameEnded = true;
	}
	if (key == Key::T && action == Action::Press) {
		Engine::instance().setStatsGraphVisibility(true);
		areStatsVisible = true;
	}
	if (key == Key::G && action == Action::Press) {
		Engine::instance().setStatsGraphVisibility(false);
		areStatsVisible = false;
	}
	if (key == Key::R && action == Action::Press) {
		Game::instance().addPlayer();
	}
	if (key == Key::F && action == Action::Press) {
		for (size_t i = 0; i < 50; i++)
		{
			Game::instance().addCollectible();
		}
	}
	if (key == Key::Space && modifier == Modifier::Shift && action == Action::Release)
	{
		Log::Info("Released space key, disconnecting");
		wsHandler->disconnect();
	}

	//Left
	if (key == Key::A && (action == Action::Press || action == Action::Repeat))
	{
		Game::instance().rotateAllPlayers(0.1f);
	}
	//Right
	if (key == Key::D && (action == Action::Press || action == Action::Repeat))
	{
		Game::instance().rotateAllPlayers(-0.1f);
	}

	if (key == Key::I && (action == Action::Press || action == Action::Repeat))
	{
		wsHandler->queueMessage("U start");

		isGameStarted = true;
		Game::instance().startGame();
		// std::string timePassed = std::to_string(Game::instance().getPassedTime());
	}
}

void preSync()
{
	// Do the application simulation step on the server node in here and make sure that
	// the computed state is serialized and deserialized in the encode/decode calls

	//Run game simulation on master only
	if (Engine::instance().isMaster())
	{
		if (!isGameEnded && isGameStarted) {
			if (Game::instance().shouldSendTime()) {
				std::string timePassed = std::to_string(Game::instance().getPassedTime());
				wsHandler->queueMessage("T " + timePassed);
			}
			Game::instance().update();
			if (Game::instance().hasGameEnded()) {
				if (!isGameEnded) {
					wsHandler->queueMessage("U end");
					isGameEnded = true;
				}
			}
		}
		wsHandler->tick();
	}
}

std::vector<std::byte> encode()
{
	std::vector<std::byte> output;

	serializeObject(output, isGameEnded);
	serializeObject(output, areStatsVisible);
	serializeObject(output, isGameStarted);

	//For some reason everything has to to be put in one vector to avoid sgct syncing bugs
	serializeObject(output, Game::instance().getSyncableData());

	return output;
}

void decode(const std::vector<std::byte>& data, unsigned int pos)
{
	if (!Game::exists() || isGameEnded) //No point in syncing data if no Game isnt running
		return;

	deserializeObject(data, pos, isGameEnded);
	deserializeObject(data, pos, areStatsVisible);
	deserializeObject(data, pos, isGameStarted);
	deserializeObject(data, pos, gameObjectStates);
}

void cleanup()
{
	// Cleanup all of your state, particularly the OpenGL state in here.  This function
	// should behave symmetrically to the initOGL function
}

void postSyncPreDraw()
{
	//Sync gameobjects' state on clients only
	if (!Engine::instance().isMaster() && Game::exists())
	{
		Engine::instance().setStatsGraphVisibility(areStatsVisible);

		if (!isGameStarted || isGameEnded)
			return;
		else if(gameObjectStates.size() > 0 && !isGameEnded) {
			Game::instance().setSyncableData(std::move(gameObjectStates));
		}
	}
	else
	{
		if (isGameStarted)
			Game::instance().sendPointsToServer(wsHandler);
	}
}

void connectionEstablished()
{
	Log::Info("Connection established");
}

void connectionClosed()
{
	Log::Info("Connection closed");
}

void messageReceived(const void* data, size_t length)
{
	std::string_view msg = std::string_view(reinterpret_cast<const char*>(data), length);
	//Log::Info("Message received: %s", msg.data());

	std::string message = msg.data();

	if (!message.empty())
	{
		//Get an easily manipulated stream of the message and read type of message
		std::istringstream iss(message);
		char msgType;
		iss >> msgType;

		// If first slot is 'N', a name and unique ID has been sent
		if (msgType == 'N') {
			Log::Info("Player connected: %s", message.c_str());
			Game::instance().addPlayer(Utility::getNewPlayerData(iss));
		}

		// If first slot is 'C', the rotation angle has been sent
		if (msgType == 'C') {
			Game::instance().updateTurnSpeed(Utility::getTurnSpeed(iss));
		}


        // If first slot is 'D', player to be deleted has been sent
        if (msgType == 'D') {
            unsigned int playerId;
            iss >> playerId;
            Log::Info("Player disabled: %s", message.c_str());
            Game::instance().disablePlayer(playerId);
        }

        // If first slot is 'E', player to be enabled has been sent
        if (msgType == 'E') {
            Log::Info("Player enabled: %s", message.c_str());
            unsigned int playerId;
            iss >> playerId;
            Game::instance().enablePlayer(playerId);
        }

        // If first slot is 'I', player's ID has been sent
        if (msgType == 'I') {
            unsigned int playerId;
            iss >> playerId;
            // Send colour information back to server
            std::pair<glm::vec3, glm::vec3> colours = Game::instance().getPlayerColours(playerId);
            std::string colourOne = glm::to_string(colours.first);
            std::string colourTwo = glm::to_string(colours.second);

            wsHandler->queueMessage("A " + colourOne + " " + std::to_string(playerId));
            wsHandler->queueMessage("B " + colourTwo + " " + std::to_string(playerId));
        }
	}
}
