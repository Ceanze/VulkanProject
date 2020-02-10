#pragma once

#include "Core/Window.h"
#include "SwapChain.h"

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
	SwapChain swapChain;

	bool running = true;
};