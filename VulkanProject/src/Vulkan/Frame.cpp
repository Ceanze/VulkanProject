#include "jaspch.h"
#include "Frame.h"

#include "Vulkan/Instance.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/CommandBuffer.h"
#include "VKImgui.h"
#include "Core/Window.h"
#include "VulkanProfiler.h"

Frame::Frame()
	: window(nullptr), imgui(nullptr), swapChain(nullptr), numImages(0), framesInFlight(0),
	currentFrame(0), imageIndex(0), dt(0.0f)
{
}

Frame::~Frame()
{
}

void Frame::init(Window* window, SwapChain* swapChain)
{
	this->swapChain = swapChain;
	this->numImages = swapChain->getNumImages();
	this->window = window;
	this->framesInFlight = 2; // Unsure of purpose
	this->currentFrame = 0;
	this->imageIndex = 0;

	this->imgui = new VKImgui();
	this->imgui->init(window, swapChain);

	createSyncObjects();
}

void Frame::cleanup()
{
	destroySyncObjects();
	this->imgui->cleanup();
	delete this->imgui;
}

void Frame::submit(VkQueue queue, CommandBuffer** commandBuffers)
{
	VulkanProfiler::get().render(this->dt);

	this->imgui->end();
	this->imgui->render();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

	VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[this->currentFrame], this->computeSemaphores[0] };
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphores;

	VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
	std::vector<VkCommandBuffer> buffers = { commandBuffers[this->imageIndex]->getCommandBuffer(), this->imgui->getCurrentCommandBuffer() };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pCommandBuffers = buffers.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = (uint32_t)buffers.size();

	vkResetFences(Instance::get().getDevice(), 1, &this->inFlightFences[this->currentFrame]);
	ERROR_CHECK(vkQueueSubmit(queue, 1, &submitInfo, this->inFlightFences[this->currentFrame]), "Failed to sumbit commandbuffer!");

	if (this->imageIndex == 0) {
		VulkanProfiler::get().getAllQueries();
	}
}

void Frame::submitCompute(VkQueue queue, CommandBuffer** commandBuffer)
{
	vkQueueWaitIdle(Instance::get().getGraphicsQueue().queue);

	// Submit compute shader for frustum culling

	// Wait for fence to ensure that compute buffer writes have finished
	//vkWaitForFences(Instance::get().getDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
	//vkResetFences(Instance::get().getDevice(), 1, &this->computeFence);

	VkSubmitInfo computeSubmitInfo = { };
	std::array<VkCommandBuffer, 1> buff = { commandBuffer[0]->getCommandBuffer() };
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmitInfo.commandBufferCount = buff.size();
	computeSubmitInfo.pCommandBuffers = buff.data();
	computeSubmitInfo.signalSemaphoreCount = 1;
	computeSubmitInfo.pSignalSemaphores = &this->computeSemaphores[0];

	ERROR_CHECK(vkQueueSubmit(queue, 1, &computeSubmitInfo, VK_NULL_HANDLE), "Failed to submit compute queue!");
}

bool Frame::beginFrame(float dt)
{
	this->dt = dt;

	vkWaitForFences(Instance::get().getDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(Instance::get().getDevice(), this->swapChain->getSwapChain(), UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &this->imageIndex);

	// Check if window has been resized
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	}
	JAS_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to aquire swap chain image!");
	
	// Check if a previous frame is using this image (Wait for this image to be free for use)
	if (this->imagesInFlight[this->imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(Instance::get().getDevice(), 1, &this->imagesInFlight[this->imageIndex], VK_TRUE, UINT64_MAX);
	}
	
	/*
		Marked image as being used by this frame
		This current frame will use the image with index imageIndex, mark this so that we now wen we can use this again. 
	*/
	this->imagesInFlight[this->imageIndex] = this->inFlightFences[this->currentFrame];

	this->imgui->begin(this->imageIndex, 0.016f);

	return true;
}

bool Frame::endFrame()
{
	VkSemaphore waitSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;

	presentInfo.swapchainCount = 1;
	VkSwapchainKHR swapchain = this->swapChain->getSwapChain();
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &this->imageIndex;

	VkResult result = vkQueuePresentKHR(Instance::get().getPresentQueue().queue, &presentInfo);

	// Check if window has been resized
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		return false;
	}
	JAS_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to present image!");

	this->currentFrame = (this->currentFrame + 1) % this->framesInFlight;
	return true;
}

uint32_t Frame::getCurrentImageIndex() const
{
	return this->imageIndex;
}

void Frame::createSyncObjects()
{
	this->imageAvailableSemaphores.resize(this->framesInFlight);
	this->renderFinishedSemaphores.resize(this->framesInFlight);
	this->computeSemaphores.resize(this->framesInFlight);
	this->inFlightFences.resize(this->framesInFlight);
	this->imagesInFlight.resize(this->numImages, VK_NULL_HANDLE);

	// Graphics
	for (uint32_t i = 0; i < this->framesInFlight; i++) {
		VkSemaphoreCreateInfo SemaCreateInfo = {};
		SemaCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &SemaCreateInfo, nullptr, &this->imageAvailableSemaphores[i]), "Failed to create graphics semaphore");
		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &SemaCreateInfo, nullptr, &this->renderFinishedSemaphores[i]), "Failed to create graphics semaphore");
		ERROR_CHECK(vkCreateFence(Instance::get().getDevice(), &fenceCreateInfo, nullptr, &this->inFlightFences[i]), "Failed to create graphics fence");

		ERROR_CHECK(vkCreateSemaphore(Instance::get().getDevice(), &SemaCreateInfo, nullptr, &this->computeSemaphores[i]), "Failed to create compute semaphore");
	}

	// Compute
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	ERROR_CHECK(vkCreateFence(Instance::get().getDevice(), &fenceCreateInfo, nullptr, &this->computeFence), "Failed to create graphics fence");
}

void Frame::destroySyncObjects()
{
	for (uint32_t i = 0; i < this->framesInFlight; i++) {
		vkDestroySemaphore(Instance::get().getDevice(), this->imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(Instance::get().getDevice(), this->renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(Instance::get().getDevice(), this->inFlightFences[i], nullptr);
		vkDestroySemaphore(Instance::get().getDevice(), this->computeSemaphores[i], nullptr);
	}
	vkDestroyFence(Instance::get().getDevice(), this->computeFence, nullptr);
}
