#pragma once

class Window
{
public:
	Window();
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void init();
	void cleanup();

private:
	unsigned width = 1280;
	unsigned height = 720;
};