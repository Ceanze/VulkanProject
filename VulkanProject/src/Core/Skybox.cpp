#include "jaspch.h"
#include "Skybox.h"

#include "Vulkan/CommandPool.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Instance.h"

#include "stb/stb_image.h"

Skybox::Skybox()
{

}

Skybox::~Skybox()
{
}

void Skybox::init(const std::string& texturePath, SwapChain* swapChain, CommandPool* pool)
{
	for (int i = 0; i < 36; i++)
	{
		this->cube[i] *= 100.0f;
	}
	// Add attachments
	this->renderPass.addDefaultColorAttachment(swapChain->getImageFormat());
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
	JAS_INFO("Loading cubemap: {0}", texturePath.c_str());
	for (std::string& face : faces)
	{
		std::string finalFilePath = texturePath;
		finalFilePath += face;

		int channels;
		stbi_uc* img = stbi_load(finalFilePath.c_str(), &width, &height, &channels, 4);
		data.push_back(img);
		JAS_INFO(" ->Loaded face: {0}", face.c_str());
	}

	// Create staging buffer.
	uint32_t numFaces = (uint32_t)data.size();
	uint32_t size = (uint32_t)(width * height * 4);
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->cubemapStagingBuffer.init(size * numFaces, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);
	this->cubemapStagingMemory.bindBuffer(&this->cubemapStagingBuffer);
	this->cubemapStagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer the data to the buffer.
	for (uint32_t f = 0; f < numFaces; f++)
	{
		stbi_uc* img = data[f];
		this->cubemapStagingMemory.directTransfer(&this->cubemapStagingBuffer, (void*)img, size, (Offset)f * size);
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
	desc.pool = pool;
	desc.layerCount = numFaces;
	Image& image = this->cubemapTexture.getImage();
	image.transistionLayout(desc);
	image.copyBufferToImage(&this->cubemapStagingBuffer, pool, bufferCopyRegions);
	desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image.transistionLayout(desc);

	// Stage and transfer cube to device
	{
		Buffer stagingBuffer;
		Memory stagingMemory;
		stagingBuffer.init(sizeof(this->cube), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getTransferQueue().queueIndex });
		stagingMemory.bindBuffer(&stagingBuffer);
		stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingMemory.directTransfer(&stagingBuffer, this->cube, sizeof(this->cube), 0);

		this->cubeBuffer.init(sizeof(this->cube), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, { Instance::get().getGraphicsQueue().queueIndex, Instance::get().getTransferQueue().queueIndex });
		this->cubeMemory.bindBuffer(&this->cubeBuffer);
		this->cubeMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		CommandBuffer* cmdBuff = pool->beginSingleTimeCommand();

		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = sizeof(this->cube);
		cmdBuff->cmdCopyBuffer(stagingBuffer.getBuffer(), this->cubeBuffer.getBuffer(), 1, &region);

		pool->endSingleTimeCommand(cmdBuff);
		stagingBuffer.cleanup();
		stagingMemory.cleanup();
	}
	// Create sampler
	// TODO: This needs more control, should be compareOp=VK_COMPARE_OP_NEVER!
	this->cubemapSampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	// Staging buffer and memory are not used anymore, can be destroyed.
	this->cubemapStagingBuffer.cleanup();
	this->cubemapStagingMemory.cleanup();

	// Initialize the descriptor manager.
	DescriptorLayout descLayoutCubemap;
	descLayoutCubemap.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));	// Vertices
	descLayoutCubemap.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));		// Uniforms (Only vp)
	descLayoutCubemap.init();
	DescriptorLayout descLayoutCubemap2;
	descLayoutCubemap2.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr));	// The cubemap texture and sampler
	descLayoutCubemap2.init();
	this->cubemapDescManager.addLayout(descLayoutCubemap);	// Set 0
	this->cubemapDescManager.addLayout(descLayoutCubemap2); // Set 1
	this->cubemapDescManager.init(swapChain->getNumImages());

	// Set data for the descriptor manager.
	for (uint32_t i = 0; i < static_cast<uint32_t>(swapChain->getNumImages()); i++)
	{
		this->cubemapDescManager.updateBufferDesc(0, 0, this->cubeBuffer.getBuffer(), 0, sizeof(this->cube));
		this->cubemapDescManager.updateBufferDesc(0, 1, this->cubemapUniformBuffer.getBuffer(), 0, sizeof(CubemapUboData));
		this->cubemapDescManager.updateImageDesc(1, 0, this->cubemapTexture.getImage().getLayout(), this->cubemapTexture.getVkImageView(), this->cubemapSampler.getSampler());
		this->cubemapDescManager.updateSets({ 0, 1 }, i);
	}

	// Initialize the cubemap shader.
	this->shader.addStage(Shader::Type::VERTEX, "CubemapTest\\skyboxVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "CubemapTest\\skyboxFrag.spv");
	this->shader.init();

	// Initialize the cubemap pipeline.
	PipelineInfo pipelineInfo;
	pipelineInfo.rasterizer = {};
	pipelineInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineInfo.rasterizer.lineWidth = 1.0f;
	pipelineInfo.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	this->pipeline.setPipelineInfo(PipelineInfoFlag::RASTERIZATION, pipelineInfo);
	this->pipeline.setDescriptorLayouts(this->cubemapDescManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(swapChain->getExtent(), &this->renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);

	// Model data (Not used for the cubemap but for the model to get the texture for reflection)
	//for (uint32_t i = 0; i < static_cast<uint32_t>(swapChain->getNumImages()); i++)
	//{
	//	this->cubemapDescManager.updateImageDesc(1, 0, this->cubemapTexture.getImage().getLayout(), this->cubemapTexture.getVkImageView(), this->cubemapSampler.getSampler());
	//	this->cubemapDescManager.updateSets({ 1 }, i);
	//}
}

void Skybox::draw(CommandBuffer* cmdBuff)
{
}

void Skybox::cleanup()
{
}
