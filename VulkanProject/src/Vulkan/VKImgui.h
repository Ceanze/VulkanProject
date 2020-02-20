#pragma once

#include <vector>

#include "Pipeline/PushConstants.h"

class RenderPass;
class CommandPool;
class CommandBuffer;
class Window;
class SwapChain;
class Framebuffer;
struct GLFWwindow;

class VKImgui
{
public:
	VKImgui();
	~VKImgui();

	void init(Window* window, SwapChain* swapChain);
	void cleanup();

	void begin(uint32_t frameIndex, float dt);
	void end();
	void render();

	VkCommandBuffer getCurrentCommandBuffer() const;

private:
	void createFramebuffers();
	void createCommandPoolAndBuffer();
	void createDescriptorPool();
	void initImgui();
	void createRenderPass();

	VkDescriptorPool descPool;
	SwapChain* swapChain;
	CommandPool* commandPool;
	std::vector<CommandBuffer*> commandBuffers;
	std::vector<Framebuffer*> framebuffers;
	uint32_t frameIndex;
	RenderPass* renderPass;
	std::vector<PushConstants> pushConstants;
	GLFWwindow* window;
};