#pragma once
#include <string>

struct GLFWwindow;

class Window
{
public:
	Window();
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void init(unsigned width, unsigned height, const std::string& title);
	void cleanup();

	unsigned getWidth() const { return this->width; }
	unsigned getHeight() const { return this->height; }
	bool isOpen() const { return this->open; }

	GLFWwindow* getNativeWindow() const { return this->window; }

private:
	unsigned width = 1280;
	unsigned height = 720;
	static bool open;
	std::string title;
	GLFWwindow* window = nullptr;

	// Callbacks
	static void closeWindowCallback(GLFWwindow* w);
};