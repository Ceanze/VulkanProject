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
	this->keys[key] = 1;
	//auto& it2 = this->keys.find(key);
	//if (it2 == this->keys.end())
	//	it2->second = 1;
	//else
	//{
	//	if (it2->second >= 0)
	//		it2->second++;
	//	else
	//		it2->second = 1;
	//}

	auto& it = this->toggledKeys.find(key);
	if (it == this->toggledKeys.end())
		this->toggledKeys[key] = true;
	else
		this->toggledKeys[key] = !this->toggledKeys[key];
}

void Input::setKeyReleased(int key)
{
	this->keys[key] = 0;
}

bool Input::isKeyDown(int key)
{
	return this->keys[key] > 0;
}

bool Input::isKeyToggled(int key)
{
	return this->toggledKeys[key];
}

bool Input::isKeyClicked(int key)
{
	//if()
	return false;
}

Input::Input() 
	: deltaX(0.0), deltaY(0.0)
{
}
