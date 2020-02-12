#include "jaspch.h"
#include "Frame.h"

#include "Vulkan/Instance.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/CommandBuffer.h"

void Frame::init(SwapChain* swapChain)
{
	this->swapChain = swapChain;
	this->numImages = swapChain->getNumImages();
	this->framesInFlight = 2; // Unsure of purpose
	this->currentFrame = 0;

	createSyncObjects();
}

void Frame::cleanup()
{
	destroySyncObjects();
}

void Frame::submit(VkQueue queue, CommandBuffer** commandBuffers)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[this->currentFrame] };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;

	VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
	VkCommandBuffer buffer = commandBuffers[this->imageIndex]->getCommandBuffer();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pCommandBuffers = &buffer;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;

	vkResetFences(Instance::get().getDevice(), 1, &this->inFlightFences[this->currentFrame]);
	ERROR_CHECK(vkQueueSubmit(queue, 1, &submitInfo, this->inFlightFences[this->currentFrame]), "Failed to sumbit commandbuffer!");
}

bool Frame::beginFrame()
{
	vkWaitForFences(Instance::get().getDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(Instance::get().getDevice(), this->swapChain->getSwapChain(), UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &this->imageIndex);

	// Check if window has been resized
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	}
	JAS_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to aquire swap chain image!");
	
	// Check if previous frame is using this image
	if (this->imagesInFlight[this->imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(Instance::get().getDevice(), 1, &this->imagesInFlight[this->imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Marked image as being used by this frame
	this->imagesInFlight[this->imageIndex] = this->imagesInFlight[this->currentFrame];

	return true;
}

bool Frame::endFrame()
{
	VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[this->currentFrame] };
	VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	presentInfo.swapchainCount = 1;
	VkSwapchainKHR swapchain = this->swapChain->getSwapChain();
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &this->imageIndex;

	VkResult result = vkQueuePresentKHR(Instance::get().getPresentQueue(), &presentInfo);

	// Check if window has been resized
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		return false;
	}
	JAS_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to present image!");

	this->currentFrame = (this->currentFrame + 1) % this->framesInFlight;
	return true;
}

void Frame::createSyncObjects()
{
	this->imageAvailableSemaphores.resize(this->framesInFlight);
	this->renderFinishedSemaphores.resize(this->framesInFlight);
	this->inFlightFences.resize(this->framesInFlight);
	this->imagesInFlight.resize(this->numImages, VK_NULL_HANDLE);

	for (int i = 0; i < this->framesInFlight; i++) {
		VkSemaphoreCreateInfo SemaCreateInfo = {};
		SemaCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &SemaCreateInfo, nullptr, &this->imageAvailableSemaphores[i]), "Failed to create semaphore");
		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &SemaCreateInfo, nullptr, &this->renderFinishedSemaphores[i]), "Failed to create semaphore");
		ERROR_CHECK(vkCreateFence(Instance::get().getDevice(), &fenceCreateInfo, nullptr, &this->inFlightFences[i]), "Failed to create fence");
	}
}

void Frame::destroySyncObjects()
{
	for (int i = 0; i < this->framesInFlight; i++) {
		vkDestroySemaphore(Instance::get().getDevice(), this->imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(Instance::get().getDevice(), this->renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(Instance::get().getDevice(), this->inFlightFences[i], nullptr);
	}
}
