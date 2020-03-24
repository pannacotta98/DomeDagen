#include "player.hpp"

#include "sgct/log.h"
#include <iostream>

Player::Player(const std::string& name /* position,  fler argument */)
	: mName{ name }, mPoints{ 0 }, mIsAlive{ true } {
	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());
}

Player::~Player() {
	sgct::Log::Info("Player with name=\"%s\" removed", mName.c_str());
}

void Player::update(float delta_time) {
	//velocity_ = delta_time * acceleration_;  // funkar nog ej bra just f�r v�rt spel

	GameObject::update(delta_time);
}