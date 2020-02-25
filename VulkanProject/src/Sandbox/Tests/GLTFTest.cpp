#include "jaspch.h"
#include "GLTFTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Models/GLTFLoader.h"

void GLTFTest::init()
{
	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 5.f }, { 0.f, 0.f, 0.f }, 0.8f);

	this->shader.addStage(Shader::Type::VERTEX, "gltfTestVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "gltfTestFrag.spv");
	this->shader.init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(getSwapChain()->getImageFormat());
	this->renderPass.addDefaultDepthAttachment();

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 }; // One color attachment
	subpassInfo.depthStencilAttachmentIndex = 1; // Depth attachment
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

	// Depth texture
	VkFormat depthFormat = findDepthFormat(Instance::get().getPhysicalDevice());
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->depthTexture.init(getSwapChain()->getExtent().width, getSwapChain()->getExtent().height,
		depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, queueIndices);

	// Bind image to memory.
	this->imageMemory.bindTexture(&this->depthTexture);
	this->imageMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Create image view for the depth texture.
	this->depthTexture.getImageView().init(this->depthTexture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	desc.pool = &this->graphicsCommandPool;
	this->depthTexture.getImage().transistionLayout(desc);

	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	PipelineInfo pipInfo;
	// Vertex input info
	pipInfo.vertexInputInfo = {};
	pipInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	//auto bindingDescription = Vertex::getBindingDescriptions();
	//auto attributeDescriptions = Vertex::getAttributeDescriptions();
	pipInfo.vertexInputInfo.vertexBindingDescriptionCount = 0;
	pipInfo.vertexInputInfo.pVertexBindingDescriptions = nullptr;// &bindingDescription;
	pipInfo.vertexInputInfo.vertexAttributeDescriptionCount = 0;// static_cast<uint32_t>(attributeDescriptions.size());
	pipInfo.vertexInputInfo.pVertexAttributeDescriptions = nullptr;// attributeDescriptions.data();
	this->pipeline.setPipelineInfo(PipelineInfoFlag::VERTEX_INPUT, pipInfo);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	JAS_INFO("Created Renderer!");

	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	setupPost();
}

void GLTFTest::loop(float dt)
{
	// Update view matrix
	this->memory.directTransfer(&this->bufferUniform, (void*)& this->camera->getMatrix()[0], sizeof(glm::mat4), (Offset)offsetof(UboData, vp));

	// Render
	getFrame()->beginFrame(dt);
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->cmdBuffs);
	getFrame()->endFrame();
	this->camera->update(dt);
}

void GLTFTest::cleanup()
{
	this->graphicsCommandPool.cleanup();
	this->depthTexture.cleanup();
	this->imageMemory.cleanup();
	this->bufferUniform.cleanup();
	this->memory.cleanup();
	this->model.cleanup();
	this->descManager.cleanup();
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();
	delete this->camera;
}

void GLTFTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	GLTFLoader::load(filePath, &this->model);
}

void GLTFTest::setupPost()
{
	// Set uniform data.
	UboData uboData;
	uboData.vp = this->camera->getMatrix();
	uboData.world = glm::mat4(1.0f);
	uboData.world[3][3] = 1.0f;
	uint32_t unsiformBufferSize = sizeof(UboData);

	// Create the buffer and memory
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->bufferUniform.init(unsiformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->memory.bindBuffer(&this->bufferUniform);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer the data to the buffer.
	this->memory.directTransfer(&this->bufferUniform, (void*)& uboData, unsiformBufferSize, 0);

	// Update descriptor
	for (uint32_t i = 0; i < static_cast<uint32_t>(getSwapChain()->getNumImages()); i++)
	{
		VkDeviceSize vertexBufferSize = this->model.vertices.size() * sizeof(Vertex);
		this->descManager.updateBufferDesc(0, 0, this->model.vertexBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManager.updateBufferDesc(0, 1, this->bufferUniform.getBuffer(), 0, unsiformBufferSize);
		this->descManager.updateSets({ 0 }, i);
	}

	// Record command buffers
	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		this->cmdBuffs[i] = this->graphicsCommandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		this->cmdBuffs[i]->begin(0, nullptr);
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		value.depthStencil = { 1.0f, 0 };
		clearValues.push_back(value);
		this->cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[i].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		this->cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		GLTFLoader::recordDraw(&this->model, this->cmdBuffs[i], &this->pipeline, sets, offsets);

		this->cmdBuffs[i]->cmdEndRenderPass();
		this->cmdBuffs[i]->end();
	}
}