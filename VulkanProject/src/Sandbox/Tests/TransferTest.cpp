#include "jaspch.h"
#include "TransferTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Models/GLTFLoader.h"

void TransferTest::init()
{
	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->transferCommandPool.init(CommandPool::Queue::TRANSFER, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);

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
		depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, queueIndices, 0, 1);

	// Bind image to memory.
	this->imageMemory.bindTexture(&this->depthTexture);
	this->imageMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Create image view for the depth texture.
	this->depthTexture.getImageView().init(this->depthTexture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	desc.pool = &this->graphicsCommandPool;
	this->depthTexture.getImage().transistionLayout(desc);

	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	JAS_INFO("Created Renderer!");

	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	setupPost();
}

void TransferTest::loop(float dt)
{
	// Update view matrix
	this->memory.directTransfer(&this->bufferUniform, (void*)& this->camera->getMatrix()[0], sizeof(glm::mat4), (Offset)offsetof(UboData, vp));

	// Render
	getFrame()->beginFrame(dt);
	record();
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->cmdBuffs);
	getFrame()->endFrame();
	this->camera->update(dt);
}

void TransferTest::cleanup()
{
	this->thread->join();
	delete this->thread;

	this->transferModel.cleanup();
	this->defaultModel.cleanup();
	this->stagingBuffer.cleanup();
	this->stagingMemory.cleanup();
	this->depthTexture.cleanup();
	this->imageMemory.cleanup();
	this->bufferUniform.cleanup();
	this->memory.cleanup();
	this->descManager.cleanup();
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();
	this->transferCommandPool.cleanup();
	this->graphicsCommandPool.cleanup();
	delete this->camera;
}

void TransferTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	// This can now be done in another thread.
	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	GLTFLoader::load(filePath, &this->defaultModel);
	this->isTransferDone = false;
	this->thread = new std::thread(&TransferTest::loadingThread, this);
}

void TransferTest::setupPost()
{
	// Set uniform data.
	UboData uboData;
	uboData.vp = this->camera->getMatrix();
	uboData.world = glm::translate(glm::mat4(1.0f), glm::vec3{0.0f, 0.0f, -1.0f});
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
		VkDeviceSize vertexBufferSize = this->defaultModel.vertices.size() * sizeof(Vertex);
		this->descManager.updateBufferDesc(0, 0, this->defaultModel.vertexBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManager.updateBufferDesc(0, 1, this->bufferUniform.getBuffer(), 0, unsiformBufferSize);
		this->descManager.updateSets({ 0 }, i);
	}

	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++)
		this->cmdBuffs[i] = this->graphicsCommandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void TransferTest::loadingThread()
{
	const std::string filePath = "..\\assets\\Models\\Sponza\\glTF\\Sponza.gltf";

	GLTFLoader::prepareStagingBuffer(filePath, &this->transferModel, &this->stagingBuffer, &this->stagingMemory);

	this->stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// Use the transfer queue, create a single command buffer and copy the contents of the staging buffer to the model's buffers.
	GLTFLoader::transferToModel(&this->transferCommandPool, &this->transferModel, &this->stagingBuffer, &this->stagingMemory);

	{
		std::lock_guard<std::mutex> lock(this->mutex);
		this->isTransferDone = true;
	}
}

void TransferTest::record()
{
	static bool useTransferedModel = false;
	{
		std::lock_guard<std::mutex> lock(this->mutex);
		if (this->isTransferDone)
		{
			this->isTransferDone = false;
			useTransferedModel = true;

			// Update descriptor
			for (uint32_t i = 0; i < static_cast<uint32_t>(getSwapChain()->getNumImages()); i++)
			{
				VkDeviceSize vertexBufferSize = this->transferModel.vertices.size() * sizeof(Vertex);
				this->descManager.updateBufferDesc(0, 0, this->transferModel.vertexBuffer.getBuffer(), 0, vertexBufferSize);
				this->descManager.updateSets({ 0 }, i);
			}
			// Update world matrix, because Sponza is big.
			glm::mat4 world(0.01f);
			world[3][3] = 1.0f;
			this->memory.directTransfer(&this->bufferUniform, (void*)&world[0][0], sizeof(world), offsetof(UboData, world));
		}
	}

	// Record command buffer
	uint32_t currentImage = getFrame()->getCurrentImageIndex();
	this->cmdBuffs[currentImage]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr);
	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	this->cmdBuffs[currentImage]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[currentImage].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

	std::vector<VkDescriptorSet> sets = { this->descManager.getSet(currentImage, 0) };
	std::vector<uint32_t> offsets;
	this->cmdBuffs[currentImage]->cmdBindPipeline(&this->pipeline);

		
	if (useTransferedModel)
		GLTFLoader::recordDraw(&this->transferModel, this->cmdBuffs[currentImage], &this->pipeline, sets, offsets);
	else
		GLTFLoader::recordDraw(&this->defaultModel, this->cmdBuffs[currentImage], &this->pipeline, sets, offsets);

	this->cmdBuffs[currentImage]->cmdEndRenderPass();
	this->cmdBuffs[currentImage]->end();
}
