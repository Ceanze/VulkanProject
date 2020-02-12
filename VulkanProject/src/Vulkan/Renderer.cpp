#include "jaspch.h"
#include "Renderer.h"
#include "Vulkan/Instance.h"

#include <GLFW/glfw3.h>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

	this->shader.addStage(Shader::Type::VERTEX, "testVertex.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "testFragment.spv");
	this->shader.init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(this->swapChain.getImageFormat());

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = {0};
	this->renderPass.addSubpass(subpassInfo);

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	this->renderPass.addSubpassDependency(subpassDependency);
	this->renderPass.init();

	this->pipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	JAS_INFO("Created Renderer!");

	this->commandPool.init(CommandPool::Queue::GRAPHICS);
	JAS_INFO("Created Command Pool!");

	this->framebuffers.resize(this->swapChain.getNumImages());

	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain.getImageViews()[i]);
		this->framebuffers[i].init(this->swapChain.getNumImages(), &this->renderPass, imageViews, this->swapChain.getExtent());
	}
}

void Renderer::run()
{
	CommandBuffer* cmdBuffs[3];
	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		cmdBuffs[i] = this->commandPool.createCommandBuffer();
		cmdBuffs[i]->begin();
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), {0.0f, 0.0f, 0.0f, 1.0f});
		cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		cmdBuffs[i]->cmdDraw(3, 1, 0, 0);
		cmdBuffs[i]->cmdEndRenderPass();
		cmdBuffs[i]->end();
	}

	static std::vector<VkSemaphore> imageAvailableSemaphores = {};
	static std::vector<VkSemaphore> renderFinishedSemaphores = {};

	imageAvailableSemaphores.resize(this->swapChain.getNumImages());
	renderFinishedSemaphores.resize(this->swapChain.getNumImages());

	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		VkSemaphoreCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(Instance::get().getDevice(), &createInfo, nullptr, &imageAvailableSemaphores[i]);
		vkCreateSemaphore(Instance::get().getDevice(), &createInfo, nullptr, &renderFinishedSemaphores[i]);
	}

	static uint32_t currentFrame = 0;

	while (running && this->window.isOpen())
	{
		glfwPollEvents();

		vkDeviceWaitIdle(Instance::get().getDevice());

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(Instance::get().getDevice(), this->swapChain.getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		VkCommandBuffer buffer = cmdBuffs[imageIndex]->getCommandBuffer();
		submitInfo.pCommandBuffers = &buffer;
		
		ERROR_CHECK(vkQueueSubmit(Instance::get().getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE), "Failed to sumbit commandbuffer!");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		presentInfo.swapchainCount = 1;
		VkSwapchainKHR swapchain = this->swapChain.getSwapChain();
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageIndex;

		ERROR_CHECK(vkQueuePresentKHR(Instance::get().getPresentQueue(), &presentInfo), "Failed to present image!");
		currentFrame = (currentFrame + 1) % this->swapChain.getNumImages();
	}

	vkDeviceWaitIdle(Instance::get().getDevice());

	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		vkDestroySemaphore(Instance::get().getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(Instance::get().getDevice(), renderFinishedSemaphores[i], nullptr);
	}

}

void Renderer::shutdown()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	this->commandPool.cleanup();
	for (auto& framebuffer : this->framebuffers)
		framebuffer.cleanup();
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();
	this->swapChain.cleanup();
	Instance::get().cleanup();
	this->window.cleanup();
}
