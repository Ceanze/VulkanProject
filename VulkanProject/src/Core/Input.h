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

	void updateCursor(double deltaX, double deltaY);
	void updateKey(int key, bool pressed);
private:
	Input();
	Input(Input& other) = delete;

	double deltaX, deltaY;

	std::unordered_map<int, bool> keys;
};