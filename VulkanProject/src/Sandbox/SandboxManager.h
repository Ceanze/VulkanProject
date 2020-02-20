#pragma once

#include "Core/Window.h"
#include "Vulkan/SwapChain.h"

class VKSandboxBase;
class SandboxManager
{
public:
	SandboxManager();
	~SandboxManager();

	void add(VKSandboxBase* sandbox);

	void init();
	void run();
	void cleanup();

private:
	std::vector<VKSandboxBase*> sandboxes;
	bool running;

	Window window;
	SwapChain swapChain;
};