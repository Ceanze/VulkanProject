#pragma once

#include "jaspch.h"
#include <GLFW/glfw3.h>

class Input
{
public:
	~Input();

	static Input& get();

	glm::vec2 getCursorDelta();
	bool isKeyDown(int key);
	bool isKeyToggled(int key);

	void updateCursor(double deltaX, double deltaY);
	void setKeyPressed(int key);
	void setKeyReleased(int key);
private:
	Input();
	Input(Input& other) = delete;

	double deltaX, deltaY;

	std::unordered_map<int, bool> keys;
	std::unordered_map<int, bool> toggledKeys;
};