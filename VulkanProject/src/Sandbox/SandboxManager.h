#pragma once

#include "Core/Window.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Frame.h"

class VKSandboxBase;
class SandboxManager
{
public:
	SandboxManager();
	~SandboxManager();

	void set(VKSandboxBase* sandbox);

	void init();
	void run();
	void cleanup();

private:
	VKSandboxBase* sandbox;
	bool running;

	Window window;
	SwapChain swapChain;
	Frame frame;
};