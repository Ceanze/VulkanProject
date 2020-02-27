#include "jaspch.h"
#include "CubemapTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Models/GLTFLoader.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#include <stb/stb_image.h>
#pragma warning(pop)

void CubemapTest::init()
{
	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 5.f }, { 0.f, 0.f, 0.f }, 0.8f);

	// TODO: Set this to 2!!
	getShaders().resize(1);
	getPipelines().resize(1);

	getShader(MAIN_SHADER).addStage(Shader::Type::VERTEX, "gltfTestVert.spv");
	getShader(MAIN_SHADER).addStage(Shader::Type::FRAGMENT, "gltfTestFrag.spv");
	getShader(MAIN_SHADER).init();

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

	getPipeline(MAIN_PIPELINE).setDescriptorLayouts(this->descManager.getLayouts());
	getPipeline(MAIN_PIPELINE).setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
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
	getPipeline(MAIN_PIPELINE).setPipelineInfo(PipelineInfoFlag::VERTEX_INPUT, pipInfo);
	getPipeline(MAIN_PIPELINE).init(Pipeline::Type::GRAPHICS, &getShader(MAIN_SHADER));
	JAS_INFO("Created Renderer!");

	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	setupPost();
}

void CubemapTest::loop(float dt)
{
	// Update view matrix
	this->memory.directTransfer(&this->bufferUniform, (void*)& this->camera->getMatrix()[0], sizeof(glm::mat4), (Offset)offsetof(UboData, vp));

	// Render
	getFrame()->beginFrame(dt);
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->cmdBuffs);
	getFrame()->endFrame();
	this->camera->update(dt);
}

void CubemapTest::cleanup()
{
	// Cubemap
	this->cubemapSampler.cleanup();
	this->cubemapTexture.cleanup();
	this->cubemapMemory.cleanup();

	// Other
	this->graphicsCommandPool.cleanup();
	this->depthTexture.cleanup();
	this->imageMemory.cleanup();
	this->bufferUniform.cleanup();
	this->memory.cleanup();
	this->model.cleanup();
	this->descManager.cleanup();
	for (auto& pipeline : getPipelines())
		pipeline.cleanup();
	this->renderPass.cleanup();
	for(auto& shader : getShaders())
		shader.cleanup();
	delete this->camera;
}

void CubemapTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	GLTFLoader::load(filePath, &this->model);
}

void CubemapTest::setupPost()
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

	setupCubemap();

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
		this->cmdBuffs[i]->cmdBindPipeline(&getPipeline(MAIN_PIPELINE));
		GLTFLoader::recordDraw(&this->model, this->cmdBuffs[i], &getPipeline(MAIN_PIPELINE), sets, offsets);

		this->cmdBuffs[i]->cmdEndRenderPass();
		this->cmdBuffs[i]->end();
	}
}

void CubemapTest::setupCubemap()
{
	std::vector<std::string> faces = {
		"right.jpg",
		"left.jpg",
		"top.jpg",
		"bottom.jpg",
		"front.jpg",
		"back.jpg"
	};

	// Fetch each face of the cubemap.
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	int32_t width = 0, height = 0;
	std::vector<stbi_uc*> data;
	std::string pathToCubemap = "..\\assets\\Textures\\skybox\\";
	JAS_INFO("Loading cubemap: {0}", pathToCubemap.c_str());
	for (std::string& face : faces)
	{
		std::string finalFilePath = pathToCubemap;
		finalFilePath += face;

		int channels;
		stbi_uc* img = stbi_load(finalFilePath.c_str(), &width, &height, &channels, 4);
		data.push_back(img);
		JAS_INFO(" ->Loaded face: {0}", face.c_str());
	}

	// Create staging buffer.
	uint32_t numFaces = (uint32_t)data.size();
	uint32_t size = (uint32_t)(width * height * 4);
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice())};
	this->cubemapStagineBuffer.init(size* numFaces, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);
	this->cubemapStagingMemory.bindBuffer(&this->cubemapStagineBuffer);
	this->cubemapStagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer the data to the buffer.
	for (uint32_t f = 0; f < numFaces; f++)
	{
		stbi_uc* img = data[f];
		this->cubemapStagingMemory.directTransfer(&this->cubemapStagineBuffer, (void*)img, size, (Offset)f*size);
		delete img;
	}

	// Create texture.
	this->cubemapTexture.init((uint32_t)width, (uint32_t)height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, queueIndices, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, numFaces);
	this->cubemapMemory.bindTexture(&this->cubemapTexture);
	this->cubemapMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	this->cubemapTexture.getImageView().init(this->cubemapTexture.getVkImage(), VK_IMAGE_VIEW_TYPE_CUBE, format, VK_IMAGE_ASPECT_COLOR_BIT, numFaces);

	// Setup buffer copy regions for each face including all of its miplevels. In this case only 1 miplevel is used.
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	for (uint32_t face = 0; face < numFaces; face++)
	{
		for (uint32_t level = 0; level < 1; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width >> level;
			bufferCopyRegion.imageExtent.height = height >> level;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = (VkDeviceSize)(face * size);
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
	}

	// Image barrier for optimal image (target)
	Image::TransistionDesc desc;
	desc.format = format;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.pool = &this->graphicsCommandPool;
	desc.layerCount = numFaces;
	Image& image = this->cubemapTexture.getImage();
	image.transistionLayout(desc);
	image.copyBufferToImage(&this->cubemapStagineBuffer, &this->graphicsCommandPool, bufferCopyRegions);
	desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image.transistionLayout(desc);

	// Create sampler
	// TODO: This needs more control, should be compareOp=VK_COMPARE_OP_NEVER!
	this->cubemapSampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	
	// Staging buffer and memory are not used anymore, can be destroyed.
	this->cubemapStagineBuffer.cleanup();
	this->cubemapStagingMemory.cleanup();
}
