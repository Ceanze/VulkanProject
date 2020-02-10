#pragma once

#include "Core/Window.h"

class Renderer
{
public:
	Renderer();
	~Renderer();

	void init();
	void run();
	void shutdown();

private:
	Window window;

	bool running = true;
};