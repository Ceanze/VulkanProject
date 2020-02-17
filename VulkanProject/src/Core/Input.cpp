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

void Input::updateKey(int key, bool pressed)
{
	this->keys[key] = pressed;
}

bool Input::isKeyDown(int key)
{
	return this->keys[key];
}

bool Input::isKeyToggled(int key)
{
	return this->keys[key];
}

Input::Input() 
	: deltaX(0.0), deltaY(0.0)
{
}
