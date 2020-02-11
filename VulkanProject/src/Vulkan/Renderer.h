#pragma once

#include "Core/Window.h"
#include "SwapChain.h"
#include "Pipeline/Shader.h"

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
	Shader shader;

	bool running = true;
};