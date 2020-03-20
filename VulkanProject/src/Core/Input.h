#pragma once

#include "jaspch.h"
#include <GLFW/glfw3.h>

class Input
{
public:
	enum class KeyState
	{
		RELEASED = -1,
		FIRST_RELEASED = 0,
		FIRST_PRESSED = 1,
		PRESSED = 2
	};
public:
	~Input();

	static Input& get();

	glm::vec2 getCursorDelta();
	bool isKeyDown(int key);
	bool isKeyToggled(int key);
	KeyState getKeyState(int key);

	void updateCursor(double deltaX, double deltaY);
	void setKeyPressed(int key);
	void setKeyReleased(int key);

	static std::string keyStateToStr(KeyState state);
	void update();

private:
	Input();
	Input(Input& other) = delete;

	double deltaX, deltaY;

	std::unordered_map<int, KeyState> keys;
	std::unordered_map<int, bool> toggledKeys;
};