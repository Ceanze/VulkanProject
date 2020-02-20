#pragma once

#include "Vulkan/Instance.h"
#include "SandboxManager.h"
#include "Core/Window.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Pipeline/Shader.h"
#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Buffers/Framebuffer.h"
#include "Vulkan/Frame.h"

class VKSandboxBase
{
public:
	friend SandboxManager;

	VKSandboxBase();
	virtual ~VKSandboxBase();

	void selfInit();
	void selfLoop(float dt);
	void selfCleanup();

	/*
		The manager handles the window and swap chain, and it create a frame, framebuffers and command pools.
		But the developer need to initialize the framebuffers.
	*/
	virtual void init() = 0;

	/*
		This will run each frame.
	*/
	virtual void loop(float dt) = 0;

	/*
		Cleanup every thing you have created.
	*/
	virtual void cleanup() = 0;

	/*
		Initializes the framebuffers. 
		depthAttachment is optional, set it to VK_NULL_HANDLE to disable it.
	*/
	void initFramebuffers(RenderPass* renderPass, VkImageView depthAttachment);

	Window* getWindow();
	SwapChain* getSwapChain();
	std::vector<Framebuffer>& getFramebuffers();
	Frame* getFrame();

private:
	void setWindow(Window* window);
	void setSwapChain(SwapChain* swapChain);
	void setFrame(Frame* frame);

private:
	Window* window;
	SwapChain* swapChain;
	Frame* frame;

	std::vector<Framebuffer> framebuffers;
	bool framebuffersInitialized;
};