#include "jaspch.h"
#include "ComputeTransferTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>

#include "Models/GLTFLoader.h"
#include "Models/ModelRenderer.h"

#define REGION_SIZE 8

void ComputeTransferTest::init()
{
	generateHeightmap();

	// Set queue usage
	getFrame()->queueUsage(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	this->regionCount = this->heightmap.getProximityRegionCount();

	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->transferCommandPool.init(CommandPool::Queue::TRANSFER, 0);
	this->compCommandPool.init(CommandPool::Queue::COMPUTE, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 8.0f, 2.0f);

	getShaders().resize(2);
	getPipelines().resize(2);

	getShader(MESH_SHADER).addStage(Shader::Type::VERTEX, "ComputeTransferTest\\compTransferVert.spv");
	getShader(MESH_SHADER).addStage(Shader::Type::FRAGMENT, "ComputeTransferTest\\compTransferFrag.spv");
	getShader(MESH_SHADER).init();

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

	getPipeline(MESH_PIPELINE).setDescriptorLayouts(this->descManager.getLayouts());
	getPipeline(MESH_PIPELINE).setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	//getPipeline(MESH_PIPELINE).setWireframe(true);
	getPipeline(MESH_PIPELINE).init(Pipeline::Type::GRAPHICS, &getShader(MESH_SHADER));
	JAS_INFO("Created Renderer!");

	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	//setupTemp();
	setupCompute();
	setupPost();
	buildComputeCommandBuffer();
}

void ComputeTransferTest::loop(float dt)
{
	// Update view matrix
	this->camera->update(dt);

	this->memory.directTransfer(&this->bufferUniform, (void*)& this->camera->getMatrix()[0], sizeof(glm::mat4), (Offset)offsetof(UboData, vp));

	this->compUniformMemory.directTransfer(&this->planesUBO, (void*)this->camera->getPlanes().data(), sizeof(Camera::Plane) * 6, 0);

	// Render
	getFrame()->beginFrame(dt);
	record();
	getFrame()->submitCompute(Instance::get().getComputeQueue().queue, this->compCommandBuffers);
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->cmdBuffs);
	getFrame()->endFrame();
}

void ComputeTransferTest::cleanup()
{
	this->descManagerComp.cleanup();

	this->transferModel.cleanup();
	this->defaultModel.cleanup();
	this->stagingBuffer.cleanup();
	this->stagingMemory.cleanup();
	this->depthTexture.cleanup();
	this->imageMemory.cleanup();
	this->bufferUniform.cleanup();
	this->memory.cleanup();
	this->descManager.cleanup();
	this->renderPass.cleanup();
	for (auto& pipeline : getPipelines())
		pipeline.cleanup();
	for (auto& shader : getShaders())
		shader.cleanup();
	this->transferCommandPool.cleanup();
	this->graphicsCommandPool.cleanup();
	this->compCommandPool.cleanup();
	delete this->camera;
}

void ComputeTransferTest::setupPre()
{
	// Set 0
	DescriptorLayout descLayout;
	descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	descLayout.init();
	this->descManager.addLayout(descLayout);

	this->descManager.init(getSwapChain()->getNumImages());
}

void ComputeTransferTest::setupPost()
{
	// Set uniform data.
	UboData uboData;
	uboData.vp = this->camera->getMatrix();
	uint32_t uniformBufferSize = sizeof(UboData);

	// Create the buffer and memory
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->bufferUniform.init(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->memory.bindBuffer(&this->bufferUniform);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// TEMP--------------- Create index buffer
	auto indicies = Heightmap::generateIndicies(this->heightmap.getProximityVertexDim(), REGION_SIZE);
	this->indexBufferTemp.init(indicies.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, { Instance::get().getGraphicsQueue().queueIndex });
	this->indexMemoryTemp.bindBuffer(&this->indexBufferTemp);
	this->indexMemoryTemp.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	this->indexMemoryTemp.directTransfer(&this->indexBufferTemp, indicies.data(), indicies.size() * sizeof(uint32_t), 0);

	// Transfer the data to the buffer.
	this->memory.directTransfer(&this->bufferUniform, (void*)& uboData, uniformBufferSize, 0);

	// Update descriptor
	VkDeviceSize vertexBufferSize = this->verticies.size() * sizeof(glm::vec4);
	for (uint32_t i = 0; i < static_cast<uint32_t>(getSwapChain()->getNumImages()); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->compVertBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManager.updateBufferDesc(0, 1, this->bufferUniform.getBuffer(), 0, uniformBufferSize);
		this->descManager.updateSets({ 0 }, i);
	}

	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++)
		this->cmdBuffs[i] = this->graphicsCommandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void ComputeTransferTest::setupCompute()
{

	DescriptorLayout descLayout;
	descLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // Out
	descLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // In
	descLayout.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // Planes
	descLayout.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // WorldData
	descLayout.init();
	this->descManagerComp.addLayout(descLayout);
	this->descManagerComp.init(1);

	getShader(COMP_SHADER).addStage(Shader::Type::COMPUTE, "ComputeTransferTest\\compTransferComp.spv");
	getShader(COMP_SHADER).init();

	getPipeline(COMP_PIPELINE).setDescriptorLayouts(this->descManagerComp.getLayouts());
	getPipeline(COMP_PIPELINE).init(Pipeline::Type::COMPUTE, &getShader(COMP_SHADER));

	// Verticies
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_COMPUTE_BIT, Instance::get().getPhysicalDevice()) };
	this->compVertBuffer.init(sizeof(glm::vec4) * this->verticies.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
	this->compVertMemory.bindBuffer(&this->compVertBuffer);
	this->compVertMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Indirect draw buffer
	this->indirectDrawBuffer.init(sizeof(VkDrawIndexedIndirectCommand) * this->regionCount, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{  Instance::get().getGraphicsQueue().queueIndex/*, Instance::get().getComputeQueue().queueIndex, Instance::get().getTransferQueue().queueIndex*/ });
	this->indirectDrawMemory.bindBuffer(&this->indirectDrawBuffer);
	this->indirectDrawMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Copy data to device memory
	{
		Buffer indirectStagingBuffer;
		Memory indirectMemoryStagingBuffer;
		indirectStagingBuffer.init(sizeof(VkDrawIndexedIndirectCommand) * this->regionCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getTransferQueue().queueIndex });
		indirectMemoryStagingBuffer.bindBuffer(&indirectStagingBuffer);
		indirectMemoryStagingBuffer.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		std::vector<VkDrawIndexedIndirectCommand> indirectData(this->regionCount);

		for (size_t i = 0; i < indirectData.size(); i++) {
			indirectData[i].instanceCount = 1;
			indirectData[i].firstInstance = i;
		}

		indirectMemoryStagingBuffer.directTransfer(&indirectStagingBuffer, indirectData.data(), indirectData.size() * sizeof(VkDrawIndexedIndirectCommand), 0);

		CommandBuffer* cbuff = this->graphicsCommandPool.beginSingleTimeCommand();

		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = indirectData.size() * sizeof(VkDrawIndexedIndirectCommand);
		cbuff->cmdCopyBuffer(indirectStagingBuffer.getBuffer(), this->indirectDrawBuffer.getBuffer(), 1, &region);

		// Release to compute
		cbuff->releaseBuffer(&this->indirectDrawBuffer, VK_ACCESS_TRANSFER_READ_BIT, Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

		this->graphicsCommandPool.endSingleTimeCommand(cbuff);

		indirectStagingBuffer.cleanup();
		indirectMemoryStagingBuffer.cleanup();
	}
	{
		Buffer vertStagingBuffer;
		Memory vertStagingMemory;
		vertStagingBuffer.init(sizeof(glm::vec4) * this->verticies.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getTransferQueue().queueIndex });
		vertStagingMemory.bindBuffer(&vertStagingBuffer);
		vertStagingMemory.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		vertStagingMemory.directTransfer(&vertStagingBuffer, this->verticies.data(), this->verticies.size() * sizeof(glm::vec4), 0);

		CommandBuffer* cbuff = this->transferCommandPool.beginSingleTimeCommand();

		// Copy vertex data.
		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = this->verticies.size() * sizeof(glm::vec4);
		cbuff->cmdCopyBuffer(vertStagingBuffer.getBuffer(), this->compVertBuffer.getBuffer(), 1, &region);

		this->transferCommandPool.endSingleTimeCommand(cbuff);

		vertStagingBuffer.cleanup();
		vertStagingMemory.cleanup();
	}

	// Uniforms in compute
	this->worldDataUBO.init(sizeof(WorldData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->planesUBO.init(sizeof(Camera::Plane) * 6, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->compUniformMemory.bindBuffer(&this->worldDataUBO);
	this->compUniformMemory.bindBuffer(&this->planesUBO);
	this->compUniformMemory.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	WorldData tempData;
	tempData.loadedWidth = this->heightmap.getProximityVertexDim();
	tempData.numIndicesPerReg = this->heightmap.getIndiciesPerRegion();
	tempData.regWidth = this->heightmap.getRegionSize();
	tempData.regionCount = this->heightmap.getProximityRegionCount();
	this->compUniformMemory.directTransfer(&this->worldDataUBO, &tempData, sizeof(WorldData), 0);

	queueIndices = { findQueueIndex(VK_QUEUE_TRANSFER_BIT, Instance::get().getPhysicalDevice()) };
	this->compStagingBuffer.init(sizeof(glm::vec4) * this->verticies.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);
	this->compStagingMemory.bindBuffer(&this->compStagingBuffer);
	this->compStagingMemory.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	// Update descriptor
	this->descManagerComp.updateBufferDesc(0, 0, this->indirectDrawBuffer.getBuffer(), 0, sizeof(VkDrawIndexedIndirectCommand) * this->regionCount);
	this->descManagerComp.updateBufferDesc(0, 1, this->compVertBuffer.getBuffer(), 0, sizeof(glm::vec4) * this->verticies.size());
	this->descManagerComp.updateBufferDesc(0, 2, this->worldDataUBO.getBuffer(), 0, sizeof(WorldData));
	this->descManagerComp.updateBufferDesc(0, 3, this->planesUBO.getBuffer(), 0, sizeof(Camera::Plane) * 6);
	this->descManagerComp.updateSets({ 0 }, 0);
}

void ComputeTransferTest::buildComputeCommandBuffer()
{
	this->compCommandBuffers = this->compCommandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	this->compCommandBuffers->begin(0, nullptr);

	// Add memory barrier to ensure that the indirect commands have been consumed before the compute shader updates them
	this->compCommandBuffers->acquireBuffer(&this->indirectDrawBuffer, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	this->compCommandBuffers->cmdBindPipeline(&getPipeline(COMP_PIPELINE));
	std::vector<VkDescriptorSet> sets = { this->descManagerComp.getSet(0, 0) };
	std::vector<uint32_t> offsets;
	this->compCommandBuffers->cmdBindDescriptorSets(&getPipeline(COMP_PIPELINE), 0, sets, offsets);

	// Dispatch the compute job
	// The compute shader will do the frustum culling and adjust the indirect draw calls depending on object visibility.
	this->compCommandBuffers->cmdDispatch((uint32_t)ceilf((float)this->regionCount / 16), 1, 1);

	// Add memory barrier to ensure that the compute shader has finished writing the indirect command buffer before it's consumed
	this->compCommandBuffers->releaseBuffer(&this->indirectDrawBuffer, VK_ACCESS_SHADER_WRITE_BIT,
		Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	this->compCommandBuffers->end();
}

void ComputeTransferTest::record()
{
	// Record command buffer
	uint32_t currentImage = getFrame()->getCurrentImageIndex();
	this->cmdBuffs[currentImage]->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr);

	// Acquire from compute
	this->cmdBuffs[currentImage]->acquireBuffer(&this->indirectDrawBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
		Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	this->cmdBuffs[currentImage]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[currentImage].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

	this->cmdBuffs[currentImage]->cmdBindPipeline(&getPipeline(MESH_PIPELINE));

	std::vector<VkDescriptorSet> sets = { this->descManager.getSet(currentImage, 0) };
	std::vector<uint32_t> offsets;

	this->cmdBuffs[currentImage]->cmdBindIndexBuffer(this->indexBufferTemp.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	this->cmdBuffs[currentImage]->cmdBindDescriptorSets(&getPipeline(MESH_PIPELINE), 0, sets, offsets);

	//this->cmdBuffs[currentImage]->cmdDrawIndexed(this->tempIndicies.size(), 1, 0, 0, 0);
	this->cmdBuffs[currentImage]->cmdDrawIndexedIndirect(this->indirectDrawBuffer.getBuffer(), 0, this->regionCount, sizeof(VkDrawIndexedIndirectCommand));

	this->cmdBuffs[currentImage]->cmdEndRenderPass();

	// Release to transfer
	this->cmdBuffs[currentImage]->releaseBuffer(&this->indirectDrawBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
		Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	this->cmdBuffs[currentImage]->end();
}

void ComputeTransferTest::generateHeightmap()
{
	this->heightmap.setVertexDist(1.f);
	this->heightmap.setProximitySize(16);
	this->heightmap.setMaxZ(100.f);
	this->heightmap.setMinZ(0.f);

	int width, height;
	int channels;
	std::string path = "../assets/Textures/australia.jpg";
	unsigned char* data = static_cast<unsigned char*>(stbi_load(path.c_str(), &width, &height, &channels, 1));
	if (data == nullptr)
		JAS_ERROR("Failed to load heightmap, couldn't find file!");
	else {
		JAS_INFO("Loaded heightmap: {} successfully!", path);
		this->heightmap.init({ -256.f, 0.f, -256.f }, REGION_SIZE, width, height, data);
		JAS_INFO("Initilized heightmap successfully!");
		delete[] data;
	}

	this->verticies.resize(this->heightmap.getProximityVertexDim() * this->heightmap.getProximityVertexDim());
	this->heightmap.getProximityVerticies({ 0.0, 0.0, 0.0 }, this->verticies);
}
