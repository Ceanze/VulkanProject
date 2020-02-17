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
	this->keys[key] = true;

	auto& it = this->toggledKeys.find(key);
	if (it == this->toggledKeys.end())
		this->toggledKeys[key] = true;
	else
		this->toggledKeys[key] = !this->toggledKeys[key];
}

void Input::setKeyReleased(int key)
{
	this->keys[key] = false;
}

bool Input::isKeyDown(int key)
{
	return this->keys[key];
}

bool Input::isKeyToggled(int key)
{
	return this->toggledKeys[key];
}

Input::Input() 
	: deltaX(0.0), deltaY(0.0)
{
}
