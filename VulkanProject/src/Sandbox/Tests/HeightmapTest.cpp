#include "jaspch.h"
#include "HeightmapTest.h"
#include <vulkan/vulkan.h>

void HeightmapTest::init()
{
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);

	setupDescriptors();

	getShaders().resize((unsigned)Shaders::SIZE);
	getPipelines().resize((unsigned)PipelineObject::SIZE);

	initGraphicsPipeline();
	JAS_INFO("Created Graphics Pipeline!");

	this->commandPool.init(CommandPool::Queue::GRAPHICS, 0);

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
	desc.pool = &this->commandPool;
	this->depthTexture.getImage().transistionLayout(desc);

	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	for (int i = 0; i < getSwapChain()->getNumImages(); i++) {
		cmdBuffs[i] = this->commandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		cmdBuffs[i]->begin(0, nullptr);
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		value.depthStencil = { 1.0f, 0 };
		clearValues.push_back(value);
		// Graphics 
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[i].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);
		cmdBuffs[i]->cmdBindPipeline(&getPipeline((unsigned)PO::HEIGTHMAP));
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 0) };
		sets[0] = { this->descManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		cmdBuffs[i]->cmdBindDescriptorSets(&getPipeline((unsigned)PO::HEIGTHMAP), 0, sets, offsets);
		cmdBuffs[i]->cmdBindIndexBuffer(this->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		cmdBuffs[i]->cmdDrawIndexed(this->heightmap.getVertexData(0).indicies.size(), 1, 0, 0, 0);
		cmdBuffs[i]->cmdEndRenderPass();
		cmdBuffs[i]->end();
	}
}

void HeightmapTest::loop(float dt)
{
	this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);

	getFrame()->beginFrame();

	ImGui::Begin("Height Map Test");
	ImGui::Text("This is a height map of Australia! Careful of the Macropods!");
	ImGui::End();

	getFrame()->submit(Instance::get().getGraphicsQueue().queue, cmdBuffs);
	getFrame()->endFrame();
	this->camera->update(dt);
}

void HeightmapTest::cleanup()
{
	this->heightmap.cleanup();

	this->depthTexture.cleanup();
	// Compute
	this->indexBuffer.cleanup();
	this->heightBuffer.cleanup();
	this->camBuffer.cleanup();
	this->memory.cleanup();
	this->imageMemory.cleanup();
	this->commandPool.cleanup();

	for (auto& pipeline : getPipelines())
		pipeline.cleanup();

	this->descManager.cleanup();
	this->renderPass.cleanup();

	for (auto& shader : getShaders())
		shader.cleanup();

	delete this->camera;
}

void HeightmapTest::setupDescriptors()
{
	// Set 0
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.init();
	this->descManager.addLayout(this->descLayout);

	this->descManager.init(getSwapChain()->getNumImages());

	generateHeightmapData();
	updateBufferDescs();
}

void HeightmapTest::updateBufferDescs()
{
	// Initilize buffer and memory for heightmap and camera
	Heightmap::HeightmapVertexData hmData = this->heightmap.getVertexData(0);

	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };

	VkDeviceSize sizeHM = hmData.verticies.size() * sizeof(glm::vec4);
	VkDeviceSize sizeIndex = hmData.indicies.size() * sizeof(unsigned int);
	this->indexBuffer.init(sizeIndex, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
	this->heightBuffer.init(sizeHM, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
	this->camBuffer.init(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);

	this->memory.bindBuffer(&this->indexBuffer);
	this->memory.bindBuffer(&this->heightBuffer);
	this->memory.bindBuffer(&this->camBuffer);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	this->memory.directTransfer(&this->indexBuffer, (void*)hmData.indicies.data(), sizeIndex, 0);
	this->memory.directTransfer(&this->heightBuffer, (void*)hmData.verticies.data(), sizeHM, 0);
	this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);

	// Update descriptor
	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->heightBuffer.getBuffer(), 0, sizeHM);
		this->descManager.updateBufferDesc(0, 1, this->camBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->descManager.updateSets({ 0, 1 }, i);
	}
}

void HeightmapTest::generateHeightmapData()
{
	this->heightmap.setVertexDist(0.01f);
	this->heightmap.setMaxZ(10.f);
	this->heightmap.setMinZ(0.f);
	this->heightmap.loadImage("../assets/Textures/australia.jpg");
}

void HeightmapTest::initGraphicsPipeline()
{
	getShader((unsigned)Shaders::HEIGTHMAP).addStage(Shader::Type::VERTEX, "HeightmapTest/heightmapVertex.spv");
	getShader((unsigned)Shaders::HEIGTHMAP).addStage(Shader::Type::FRAGMENT, "HeightmapTest/heightmapFragment.spv");
	getShader((unsigned)Shaders::HEIGTHMAP).init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(getSwapChain()->getImageFormat());
	this->renderPass.addDefaultDepthAttachment();

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
	subpassInfo.depthStencilAttachmentIndex = 1;
	this->renderPass.addSubpass(subpassInfo);

	this->renderPass.addDefaultSubpassDependency();
	this->renderPass.init();

	std::vector<DescriptorLayout> setLayout = { this->descManager.getLayouts()[0] };
	getPipeline((unsigned)PO::HEIGTHMAP).setDescriptorLayouts(setLayout);

	getPipeline((unsigned)PO::HEIGTHMAP).setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	getPipeline((unsigned)PO::HEIGTHMAP).init(Pipeline::Type::GRAPHICS, &getShader((unsigned)Shaders::HEIGTHMAP));
}
