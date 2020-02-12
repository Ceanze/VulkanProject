#include "jaspch.h"
#include "Frame.h"

#include "Vulkan/Instance.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/CommandBuffer.h"

void Frame::init(SwapChain* swapChain)
{
	this->swapChain = swapChain;
	this->numImages = swapChain->getNumImages();
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

	ERROR_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to sumbit commandbuffer!");

}

void Frame::beginFrame()
{
	vkDeviceWaitIdle(Instance::get().getDevice());
	vkAcquireNextImageKHR(Instance::get().getDevice(), this->swapChain->getSwapChain(), UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &this->imageIndex);
}

void Frame::endFrame()
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

	ERROR_CHECK(vkQueuePresentKHR(Instance::get().getPresentQueue(), &presentInfo), "Failed to present image!");
	this->currentFrame = (this->currentFrame + 1) % this->numImages;
}

void Frame::createSyncObjects()
{
	this->imageAvailableSemaphores.resize(this->numImages);
	this->renderFinishedSemaphores.resize(this->numImages);

	for (int i = 0; i < this->numImages; i++) {
		VkSemaphoreCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &createInfo, nullptr, &this->imageAvailableSemaphores[i]), "Failed to create semaphore");
		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &createInfo, nullptr, &this->renderFinishedSemaphores[i]), "Failed to create semaphore");
	}
}

void Frame::destroySyncObjects()
{
	for (int i = 0; i < this->numImages; i++) {
		vkDestroySemaphore(Instance::get().getDevice(), this->imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(Instance::get().getDevice(), this->renderFinishedSemaphores[i], nullptr);
	}
}
