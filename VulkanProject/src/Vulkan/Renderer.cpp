#include "jaspch.h"
#include "Renderer.h"
#include "Vulkan/Instance.h"
#include "Core/Input.h"

#include <GLFW/glfw3.h>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

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

	this->camera = new Camera(this->window.getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

	this->shader.addStage(Shader::Type::VERTEX, "quadVertex.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "quadFragment.spv");
	this->shader.init();

	// Add attachments
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = this->swapChain.getImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	this->renderPass.addColorAttachment(colorAttachment);

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = {0};
	this->renderPass.addSubpass(subpassInfo);

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	this->renderPass.addSubpassDependency(subpassDependency);
	this->renderPass.init();

	setupPreTEMP();
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	

	JAS_INFO("Created Renderer!");

	this->commandPool.init(CommandPool::Queue::GRAPHICS);
	JAS_INFO("Created Command Pool!");

	// ------------------------ Offscreen setup ------------------------ 
#define OFFSCREEN
#ifdef OFFSCREEN
	colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	this->offScreenRenderPass.addColorAttachment(colorAttachment);

	subpassInfo.colorAttachmentIndices = { 0 };
	this->offScreenRenderPass.addSubpass(subpassInfo);

	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	this->offScreenRenderPass.addSubpassDependency(subpassDependency);
	this->offScreenRenderPass.init();

	this->offscreenPipeline.setDescriptorLayouts(this->offScreenDescManager.getLayouts());
	this->offscreenPipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->offScreenRenderPass);

	this->offscreenShader.addStage(Shader::Type::VERTEX, "testVertex.spv");
	this->offscreenShader.addStage(Shader::Type::FRAGMENT, "testFragment.spv");
	this->offscreenShader.init();

	this->offscreenPipeline.init(Pipeline::Type::GRAPHICS, &this->offscreenShader);

	this->offScreenTextures.resize(this->swapChain.getNumImages());
	this->offScreenFramebuffers.resize(this->swapChain.getNumImages());
	this->samplers.resize(this->swapChain.getNumImages());

	VkExtent2D extent = this->swapChain.getExtent();
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };


	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		this->samplers[i].init(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

		this->offScreenTextures[i].init(extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, queueIndices);

		this->memoryTexture.bindTexture(&this->offScreenTextures[i]);
	}
	this->memoryTexture.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// All images have to be bound before imageView can be initilized
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++) {
		Image::TransistionDesc desc;
		desc.format = VK_FORMAT_R8G8B8A8_UNORM;
		desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc.pool = &this->commandPool;
		this->offScreenTextures[i].getImage().transistionLayout(desc);

		this->offScreenTextures[i].getImageView().init(this->offScreenTextures[i].getVkImage(), VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->offScreenTextures[i].getImageView().getImageView());

		this->offScreenFramebuffers[i].init(this->swapChain.getNumImages(), &this->offScreenRenderPass, imageViews, this->swapChain.getExtent());
	}
#endif
	 
	this->framebuffers.resize(this->swapChain.getNumImages());
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain.getImageViews()[i]);
		this->framebuffers[i].init(this->swapChain.getNumImages(), &this->renderPass, imageViews, this->swapChain.getExtent());
	}

	this->frame.init(&this->swapChain);

	setupPostTEMP();
}

void Renderer::run()
{
	CommandBuffer* cmdBuffs[3];
	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		cmdBuffs[i] = this->commandPool.createCommandBuffer();
		cmdBuffs[i]->begin(0);
		std::vector<uint32_t> offsets;
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);

		// Offscreen
		cmdBuffs[i]->cmdBeginRenderPass(&this->offScreenRenderPass, this->offScreenFramebuffers[i].getFramebuffer(), this->swapChain.getExtent(), clearValues);
		cmdBuffs[i]->cmdBindPipeline(&this->offscreenPipeline);
		std::vector<VkDescriptorSet> sets = { this->offScreenDescManager.getSet(i, 0) };
		cmdBuffs[i]->cmdBindDescriptorSets(&this->offscreenPipeline, 0, sets, offsets);
		cmdBuffs[i]->cmdDraw(3, 1, 0, 0);
		cmdBuffs[i]->cmdEndRenderPass();

		// Post process
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), clearValues);
		cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		std::vector<VkDescriptorSet> sets2 = { this->descManager.getSet(i, 0) };
		cmdBuffs[i]->cmdBindDescriptorSets(&this->pipeline, 0, sets2, offsets);
		cmdBuffs[i]->cmdDraw(3, 1, 0, 0);
		cmdBuffs[i]->cmdEndRenderPass();

		cmdBuffs[i]->end();
	}

	auto currentTime = std::chrono::high_resolution_clock::now();
	auto prevTime = currentTime;
	float dt = 0;
	unsigned frames = 0;
	double elapsedTime = 0;

	while (running && this->window.isOpen())
	{
		glfwPollEvents();

		this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);

		this->frame.beginFrame();
		this->frame.submit(Instance::get().getGraphicsQueue(), cmdBuffs);
		this->frame.endFrame();
		this->camera->update(dt);
		
		currentTime = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float>(currentTime - prevTime).count();
		elapsedTime += dt;
		frames++;
		prevTime = currentTime;

		if (elapsedTime >= 1.0) {
			this->window.setTitle("FPS: " + std::to_string(frames) + " [Delta time: " + std::to_string((elapsedTime / frames) * 1000.f) + " ms]");
			elapsedTime = 0;
			frames = 0;
		}
	}
}

void Renderer::shutdown()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	this->buffer.cleanup();
	this->camBuffer.cleanup();
	this->stagingBuffer.cleanup();
	this->memory.cleanup();
	this->descManager.cleanup();
	this->texture.cleanup();
	this->sampler.cleanup();
	this->memoryTexture.cleanup();
	this->memoryTexture2.cleanup();

	// ----------------- Offscreen things -------------
#ifdef OFFSCREEN
	for (auto& framebuffer : this->offScreenFramebuffers)
		framebuffer.cleanup();
	for (auto& texture : this->offScreenTextures)
		texture.cleanup();
	for (auto& sampler : this->samplers)
		sampler.cleanup();
	this->offscreenPipeline.cleanup();
	this->offScreenRenderPass.cleanup();
	this->offscreenShader.cleanup();
	this->offScreenDescManager.cleanup();
	this->offScreenBuffer.cleanup();
#endif

	this->frame.cleanup();
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

void Renderer::setupPreTEMP()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr));
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());

	this->offScreenDescLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->offScreenDescLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->offScreenDescLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr));
	this->offScreenDescLayout.init();

	this->offScreenDescManager.addLayout(this->offScreenDescLayout);
	this->offScreenDescManager.init(this->swapChain.getNumImages());
}

void Renderer::setupPostTEMP()
{
	// Offscreen - Scene 
	glm::vec2 uvs[3] = { {0.0f, 2.0f}, {0.0f, 0.0f}, {2.0f, 0.0f} };
	glm::vec2 position[3] = { {0.0f, -0.5f}, {-0.5f, 0.0f}, {0.5f, 0.0f} };

	// Presentation - Fullscreen Triangle
	glm::vec2 triUvs[3] = { {0.0f, 2.0f}, {0.0f, 0.0f}, {2.0f, 0.0f} };
	glm::vec2 triPos[3] = { {-1.0f, 3.0f}, {-1.0f, -1.0f}, {3.0f, -1.0f} };
	uint32_t size = sizeof(glm::vec2) * 3;
	uint32_t size2 = sizeof(glm::vec2) * 3;

	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };

	int width, height;
	int channels;
	stbi_uc* img = stbi_load("..\\assets\\Textures\\svenskt.jpg", &width, &height, &channels, 4);

	// Create texture and sampler
	this->texture.init(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, queueIndices);
	
	// Create buffer
	this->offScreenBuffer.init(size + size2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
	this->buffer.init(size + size2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
	this->camBuffer.init(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->stagingBuffer.init(width * height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);

	// Create memory
	this->memory.bindBuffer(&this->offScreenBuffer);
	this->memory.bindBuffer(&this->buffer);
	this->memory.bindBuffer(&this->stagingBuffer);
	this->memory.bindBuffer(&this->camBuffer);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	this->memoryTexture2.bindTexture(&this->texture);
	this->memoryTexture2.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	this->texture.getImageView().init(this->texture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	this->sampler.init(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	// Update buffer
	this->memory.directTransfer(&this->offScreenBuffer, (void*)&uvs[0], size, 0);
	this->memory.directTransfer(&this->offScreenBuffer, (void*)&position[0], size2, size);
	this->memory.directTransfer(&this->buffer, (void*)&triUvs[0], size, 0);
	this->memory.directTransfer(&this->buffer, (void*)&triPos[0], size2, size);
	this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);
	this->memory.directTransfer(&this->stagingBuffer, (void*)img, width * height * 4, 0);

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.pool = &this->commandPool;
	Image& image = this->texture.getImage();
	image.transistionLayout(desc);
	image.copyBufferToImage(&this->stagingBuffer, &this->commandPool);
	desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image.transistionLayout(desc);

	// Update descriptor
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->buffer.getBuffer(), 0, size + size2);
		this->descManager.updateImageDesc(0, 1, this->offScreenTextures[i].getImage().getLayout(), this->offScreenTextures[i].getVkImageView(), this->samplers[i].getSampler());
		this->descManager.updateSets(i);

		this->offScreenDescManager.updateBufferDesc(0, 0, this->offScreenBuffer.getBuffer(), 0, size + size2);
		this->offScreenDescManager.updateBufferDesc(0, 1, this->camBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->offScreenDescManager.updateImageDesc(0, 2, this->texture.getImage().getLayout(), this->texture.getVkImageView(), this->sampler.getSampler());
		this->offScreenDescManager.updateSets(i);
	}
}
