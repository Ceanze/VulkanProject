#include "jaspch.h"
#include "RenderTest.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#include <stb/stb_image.h>
#pragma warning(pop)

#include <imgui.h>

void RenderTest::init()
{
	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, 0);
	this->transferCommandPool.init(CommandPool::Queue::TRANSFER, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);

	this->shader.addStage(Shader::Type::VERTEX, "testVertex.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "testFragment.spv");
	this->shader.init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(getSwapChain()->getImageFormat());

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
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

	setupPre();
	this->pipeline.setPushConstants(this->pushConstants);
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	JAS_INFO("Created Renderer!");

	initFramebuffers(&this->renderPass, VK_NULL_HANDLE);

	setupPost();
}

void RenderTest::loop(float dt)
{
	this->memory.directTransfer(&this->camBuffer, (void*)& this->camera->getMatrix()[0], sizeof(glm::mat4), 0);
	this->camera->update(dt);

	getFrame()->beginFrame();

	ImGui::Begin("Hello world!");
	ImGui::Text("Cool text");
	ImGui::End();

	getFrame()->submit(Instance::get().getGraphicsQueue().queue, cmdBuffs);
	getFrame()->endFrame();
}

void RenderTest::cleanup()
{
	this->graphicsCommandPool.cleanup();
	this->transferCommandPool.cleanup();
	this->buffer.cleanup();
	this->camBuffer.cleanup();
	this->stagingBuffer.cleanup();
	this->memory.cleanup();
	this->descManager.cleanup();
	this->texture.cleanup();
	this->sampler.cleanup();
	this->memoryTexture.cleanup();
	
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();

	delete this->camera;
}

void RenderTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr));
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	PushConstants pushConsts;
	pushConsts.setLayout(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4), 0);
	this->pushConstants.push_back(pushConsts);
}

void RenderTest::setupPost()
{
	int width, height;
	int channels;
	stbi_uc* img = stbi_load("..\\assets\\Textures\\svenskt.jpg", &width, &height, &channels, 4);
	glm::vec2 uvs[3] = { {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f} };
	glm::vec2 position[3] = { {0.0f, -0.5f}, {0.0f, 0.0f}, {0.5f, 0.0f} };
	uint32_t size = sizeof(glm::vec2) * 3;
	uint32_t size2 = sizeof(glm::vec2) * 3;

	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()),
		findQueueIndex(VK_QUEUE_TRANSFER_BIT, Instance::get().getPhysicalDevice()) };

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
	this->memory.directTransfer(&this->buffer, (void*)& uvs[0], size, 0);
	this->memory.directTransfer(&this->buffer, (void*)& position[0], size2, size);
	this->memory.directTransfer(&this->camBuffer, (void*)& this->camera->getMatrix()[0], sizeof(glm::mat4), 0);
	this->memory.directTransfer(&this->stagingBuffer, (void*)img, width * height * 4, 0);

	// Free img after transfer
	delete img;

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.pool = &this->graphicsCommandPool;
	Image & image = this->texture.getImage();
	image.transistionLayout(desc);
	image.copyBufferToImage(&this->stagingBuffer, &this->transferCommandPool);
	desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image.transistionLayout(desc);

	// Update descriptor
	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->buffer.getBuffer(), 0, size + size2);
		this->descManager.updateBufferDesc(0, 1, this->camBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->descManager.updateImageDesc(0, 2, image.getLayout(), this->texture.getVkImageView(), this->sampler.getSampler());
		this->descManager.updateSets(i);
	}

	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		cmdBuffs[i] = this->graphicsCommandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		cmdBuffs[i]->begin(0, nullptr);
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[i].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);
		cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		cmdBuffs[i]->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
		const glm::vec4 tintData(0.3f, 0.4f, 0.4f, 1.0f);
		this->pushConstants[0].setDataPtr(&tintData[0]);
		cmdBuffs[i]->cmdPushConstants(&this->pipeline, &this->pushConstants[0]);
		cmdBuffs[i]->cmdDraw(3, 1, 0, 0);
		cmdBuffs[i]->cmdEndRenderPass();
		cmdBuffs[i]->end();
	}
}
