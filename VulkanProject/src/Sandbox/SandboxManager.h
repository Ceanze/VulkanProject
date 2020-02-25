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
	std::vector<VKSandboxBase*> sandboxes;
	uint32_t currentSandbox;
	bool running;

	Window window;
	SwapChain swapChain;
	Frame frame;
};