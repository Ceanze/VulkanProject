#pragma once

#include <jaspch.h>
#include <vulkan/vulkan.h>

class SwapChain;
class CommandBuffer;
class VKImgui;
class Window;

class Frame
{
public:
	Frame();
	~Frame();


	void init(Window* window, SwapChain* swapChain);
	void cleanup();

	void submit(VkQueue queue, CommandBuffer** commandBuffers);
	void submitCompute(VkQueue queue, CommandBuffer* commandBuffer);
	void submitTransfer(VkQueue queue, CommandBuffer* commandBuffer);

	bool beginFrame(float dt);
	bool endFrame();

	uint32_t getCurrentImageIndex() const;

	void queueUsage(VkQueueFlags queueFlags);

private:
	void createSyncObjects();
	void destroySyncObjects();

	VkQueueFlags queueFlags;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkSemaphore> computeSemaphores;
	std::vector<VkSemaphore> transferSemaphores;

	VkFence computeFence;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	uint32_t framesInFlight;
	uint32_t currentFrame;
	uint32_t imageIndex;
	uint32_t numImages;
	float dt;
	SwapChain* swapChain;

	VKImgui* imgui;
	Window* window;
};