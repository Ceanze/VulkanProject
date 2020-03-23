#include "jaspch.h"
#include "Input.h"

Input::~Input()
{
}

Input& Input::get()
{
	static Input singleton;
	return singleton;
}

glm::vec2 Input::getCursorDelta()
{
	glm::vec2 cursorDelta(this->deltaX, this->deltaY);
	this->deltaX = 0;
	this->deltaY = 0;

	return cursorDelta;
}

void Input::updateCursor(double deltaX, double deltaY)
{
	this->deltaX = deltaX;
	this->deltaY = deltaY;
}

void Input::setKeyPressed(int key)
{
	auto& it = this->keys.find(key);
	if(it == this->keys.end())
		this->keys[key] = KeyState::PRESSED;
	else
		it->second = KeyState::FIRST_PRESSED;

	// Toggle key
	auto& it2 = this->toggledKeys.find(key);
	if (it2 == this->toggledKeys.end())
		this->toggledKeys[key] = true;
	else
		this->toggledKeys[key] = !this->toggledKeys[key];
}

void Input::setKeyReleased(int key)
{
	auto& it = this->keys.find(key);
	if (it == this->keys.end())
		this->keys[key] = KeyState::RELEASED;
	else
		it->second = KeyState::FIRST_RELEASED;
}

std::string Input::keyStateToStr(KeyState state)
{
	std::string str;
	switch (state)
	{
	case Input::KeyState::RELEASED:
		str = "RELEASED";
		break;
	case Input::KeyState::FIRST_RELEASED:
		str = "FIRST_RELEASED";
		break;
	case Input::KeyState::PRESSED:
		str = "PRESSED";
		break;
	case Input::KeyState::FIRST_PRESSED:
		str = "FIRST_PRESSED";
		break;
	default:
		str = "UNKNOWN_STATE";
		break;
	}
	return str;
}

void Input::update()
{
	for (auto& key : this->keys)
	{
		if (key.second == KeyState::FIRST_PRESSED) 
			key.second = KeyState::PRESSED;

		if (key.second == KeyState::FIRST_RELEASED)
			key.second = KeyState::RELEASED;
	}
}

bool Input::isKeyDown(int key)
{
	return this->keys[key] == KeyState::FIRST_PRESSED || this->keys[key] == KeyState::PRESSED;
}

bool Input::isKeyToggled(int key)
{
	return this->toggledKeys[key];
}

Input::KeyState Input::getKeyState(int key)
{
	auto& it = this->keys.find(key);
	if (it == this->keys.end())
		return KeyState::RELEASED;
	else return it->second;
}

Input::Input() 
	: deltaX(0.0), deltaY(0.0)
{
}
