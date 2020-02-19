#pragma once

#include <jaspch.h>
#include <vulkan/vulkan.h>

class SwapChain;
class CommandBuffer;

class Frame
{
public:
	void init(SwapChain* swapChain);
	void cleanup();

	void submit(VkQueue queue, CommandBuffer** commandBuffers);

	bool beginFrame();
	bool endFrame();

	uint32_t getCurrentImageIndex() const;

private:
	void createSyncObjects();
	void destroySyncObjects();

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	uint32_t framesInFlight;
	uint32_t currentFrame;
	uint32_t imageIndex;
	uint32_t numImages;
	SwapChain* swapChain;
};