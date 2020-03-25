#include "player.hpp"

#include "sgct/log.h"
#include <iostream>

Player::Player(const unsigned objType, const glm::vec2 position, const float orientation, const std::string& name /* position, ... fler argument */)
	:	GameObject{ objType, position, orientation }, mName { name }, mPoints{ 0 }, mIsAlive{ true }
{
	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());
}

Player::~Player() {
	sgct::Log::Info("Player with name=\"%s\" removed", mName.c_str());
}

//void Player::update(float deltaTime) const {
	//velocity_ = deltaTime * acceleration_;  // funkar nog ej bra just f�r v�rt spel

	//GameObject::update(deltaTime);
//}

void Player::render() const
{
	//Do something
}