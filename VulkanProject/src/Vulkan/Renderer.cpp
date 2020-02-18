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

	setupPreTEMP();
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
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

	this->frame.init(&this->swapChain);

	setupPostTEMP();
}

void Renderer::run()
{
	CommandBuffer* cmdBuffs[3];
	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		cmdBuffs[i] = this->commandPool.createCommandBuffer();
		cmdBuffs[i]->begin(0);
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), clearValues);
		cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		cmdBuffs[i]->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
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
		this->frame.submit(Instance::get().getGraphicsQueue().queue, cmdBuffs);
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
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr));
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());
}

void Renderer::setupPostTEMP()
{
	int width, height;
	int channels;
	stbi_uc* img = stbi_load("..\\assets\\Textures\\svenskt.jpg", &width, &height, &channels, 4);
	glm::vec2 uvs[3] = { {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f} };
	glm::vec2 position[3] = { {0.0f, -0.5f}, {0.0f, 0.0f}, {0.5f, 0.0f} };
	uint32_t size = sizeof(glm::vec2) * 3;
	uint32_t size2 = sizeof(glm::vec2) * 3;

	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };

	// Create texture and sampler
	this->texture.init(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, queueIndices);
	this->sampler.init(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	// Create buffer
	this->buffer.init(size + size2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
	this->camBuffer.init(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->stagingBuffer.init(width * height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);

	// Create memory
	this->memory.bindBuffer(&this->buffer);
	this->memory.bindBuffer(&this->stagingBuffer);
	this->memory.bindBuffer(&this->camBuffer);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	this->memoryTexture.bindTexture(&this->texture);
	this->memoryTexture.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	this->texture.getImageView().init(this->texture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	// Update buffer
	this->memory.directTransfer(&this->buffer, (void*)&uvs[0], size, 0);
	this->memory.directTransfer(&this->buffer, (void*)&position[0], size2, size);
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
		this->descManager.updateBufferDesc(0, 1, this->camBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->descManager.updateImageDesc(0, 2, image.getLayout(), this->texture.getVkImageView(), this->sampler.getSampler());
		this->descManager.updateSets(i);
	}
}
