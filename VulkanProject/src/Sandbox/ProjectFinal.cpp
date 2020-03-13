#include "jaspch.h"
#include "ProjectFinal.h"

#include <stb/stb_image.h>

#include "Core/Camera.h"
#include "Threading/ThreadDispatcher.h"
#include "Threading/ThreadManager.h"

#define MAIN_THREAD 0

void ProjectFinal::init()
{
	// Initalize with maximum available threads
	ThreadDispatcher::init(2);
	ThreadManager::init(static_cast<uint32_t>(std::thread::hardware_concurrency()));
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 20.f, 10.f }, { 0.f, 0.f, 0.f }, 10.0f, 4.0f);

	setupHeightmap();
	setupDescLayouts();
	setupGeneral();
	setupBuffers();
	setupMemories();
	setupDescManagers();
	setupCommandBuffers();

	transferInitalData();
}

void ProjectFinal::loop(float dt)
{
	// Update view matrix
	this->camera->update(dt);

	this->skybox.update(this->camera);

	this->memories[MEMORY_HOST_VISIBLE].directTransfer(&this->buffers[BUFFER_CAMERA], (void*)&this->camera->getMatrix()[0], this->buffers[BUFFER_CAMERA].getSize(), 0);
	this->memories[MEMORY_HOST_VISIBLE].directTransfer(&this->buffers[BUFFER_PLANES], (void*)this->camera->getPlanes().data(), this->buffers[BUFFER_PLANES].getSize(), 0);

	// Render
	getFrame()->beginFrame(dt);
	record(getFrame()->getCurrentImageIndex());
	getFrame()->submitCompute(Instance::get().getComputeQueue().queue, this->computePrimary[getFrame()->getCurrentImageIndex()]);
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->graphicsPrimary.data());
	getFrame()->endFrame();
}

void ProjectFinal::cleanup()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	ThreadDispatcher::shutdown();
	ThreadManager::cleanup();

	for (auto& descManager : this->descManagers)
		descManager.second.cleanup();

	for (auto& pool : this->graphicsPools)
		pool.cleanup();

	//for (auto& buffers : this->graphicsSecondary)
	//	for (auto& buffer : buffers.second)
	//		buffer->cleanup();

	//for (auto& buffers : this->computeSecondary)
	//	for (auto& buffer : buffers.second)
	//		buffer->cleanup();

	//for (auto& buffer : this->graphicsPrimary)
	//	buffer->cleanup();

	//for (auto& buffer : this->computePrimary)
	//	buffer->cleanup();

	for (auto& pool : this->computePools)
		pool.cleanup();

	for (auto& pool : this->transferPools)
		pool.cleanup();

	for (auto& buffer : this->buffers)
		buffer.second.cleanup();

	for (auto& memory : this->memories)
		memory.second.cleanup();


	this->depthTexture.cleanup();
	this->renderPass.cleanup();
	this->skybox.cleanup();
	this->vertices.clear();

	for (auto& pipeline : getPipelines())
		pipeline.cleanup();
	for (auto& shader : getShaders())
		shader.cleanup();

	delete this->camera;
}

void ProjectFinal::setupHeightmap()
{
	this->regionSize = 16;

	this->heightmap.setVertexDist(1.0f);
	this->heightmap.setProximitySize(60);
	this->heightmap.setMaxZ(100.f);
	this->heightmap.setMinZ(0.f);

	int width, height;
	int channels;
	std::string path = "../assets/Textures/ireland.jpg";
	unsigned char* data = static_cast<unsigned char*>(stbi_load(path.c_str(), &width, &height, &channels, 1));
	if (data == nullptr)
		JAS_ERROR("Failed to load heightmap, couldn't find file!");
	else {
		JAS_INFO("Loaded heightmap: {} successfully!", path);
		this->heightmap.init({ -width / 2.f, 0.f, -height / 2.f }, this->regionSize, width, height, data);
		JAS_INFO("Initilized heightmap successfully!");
		delete[] data;
	}

	// Set data used for transfer
	this->lastRegionIndex = this->heightmap.getRegionFromPos(this->camera->getPosition());
	this->transferThreshold = 10;

	this->regionCount = this->heightmap.getProximityRegionCount();

	this->vertices.resize(this->heightmap.getProximityVertexDim() * this->heightmap.getProximityVertexDim());
	this->heightmap.getProximityVerticies(this->camera->getPosition(), this->vertices);
}

void ProjectFinal::setupDescLayouts()
{
	// Graphics: Set 0
	{
		DescriptorLayout descLayout;
		descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
		descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Camera
		descLayout.init();
		this->descManagers[PIPELINE_GRAPHICS].addLayout(descLayout);
		this->descManagers[PIPELINE_GRAPHICS].init(getSwapChain()->getNumImages());
	}

	// Frustum compute: Set 0
	{
		DescriptorLayout descLayout;
		descLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // Out
		descLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // In
		descLayout.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // Planes
		descLayout.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // WorldData
		descLayout.init();
		this->descManagers[PIPELINE_FRUSTUM].addLayout(descLayout);
		this->descManagers[PIPELINE_FRUSTUM].init(getSwapChain()->getNumImages());
	}

	// Index compute: Set 0
	{
		DescriptorLayout descLayout;
		descLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // Index
		descLayout.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr)); // Config
		descLayout.init();
		this->descManagers[PIPELINE_INDEX].addLayout(descLayout);
		this->descManagers[PIPELINE_INDEX].init(1);
	}

}

void ProjectFinal::setupGeneral()
{
	getShaders().resize(PIPELINE_COUNT);
	getPipelines().resize(PIPELINE_COUNT);

	setupCommandPools();
	setupShaders();
	setupGraphicsPipeline();
	setupFrustumPipeline();
	setupIndexPipeline();

	// Setup depth texture
	{
		VkFormat depthFormat = findDepthFormat(Instance::get().getPhysicalDevice());
		this->depthTexture.init(getSwapChain()->getExtent().width, getSwapChain()->getExtent().height,
			depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { Instance::get().getGraphicsQueue().queueIndex }, 0, 1);

		// Bind image to memory.
		this->memories[MEMORY_TEXTURE].bindTexture(&this->depthTexture);
		this->memories[MEMORY_TEXTURE].init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Create image view for the depth texture.
		this->depthTexture.getImageView().init(this->depthTexture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		// Transistion image
		Image::TransistionDesc desc;
		desc.format = findDepthFormat(Instance::get().getPhysicalDevice());
		desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		desc.pool = &this->graphicsPools[MAIN_THREAD];
		this->depthTexture.getImage().transistionLayout(desc);
	}

	// Framebuffers
	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	// Skybox
	{
		std::string pathToCubemap = "..\\assets\\Textures\\skybox\\";
		this->skybox.init(500.0f, pathToCubemap, getSwapChain(), &this->graphicsPools[MAIN_THREAD], &this->renderPass);
	}
}

void ProjectFinal::setupBuffers()
{
	// Graphics buffers
	{
		std::vector<uint32_t> queueIndices = { Instance::get().getGraphicsQueue().queueIndex };
		this->buffers[BUFFER_CAMERA].init(sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_CAMERA]);
	}

	// Frustum buffers
	{
		// Planes and world data
		std::vector<uint32_t> queueIndices = { Instance::get().getComputeQueue().queueIndex };
		this->buffers[BUFFER_PLANES].init(sizeof(Camera::Plane) * 6, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
		this->buffers[BUFFER_WORLD_DATA].init(sizeof(WorldData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_PLANES]);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_WORLD_DATA]);

		// Indirect draw data
		this->buffers[BUFFER_INDIRECT_DRAW].init(sizeof(VkDrawIndexedIndirectCommand) * this->regionCount, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{ Instance::get().getGraphicsQueue().queueIndex/*, Instance::get().getComputeQueue().queueIndex, Instance::get().getTransferQueue().queueIndex*/ });
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_INDIRECT_DRAW]);
	
		// Compute vertex data
		this->buffers[BUFFER_VERTICES].init(sizeof(Heightmap::Vertex) * this->vertices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->buffers[BUFFER_VERTICES_2].init(sizeof(Heightmap::Vertex) * this->vertices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_VERTICES]);
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_VERTICES_2]);
	
		// Vert staging
		this->buffers[BUFFER_VERT_STAGING].init(sizeof(Heightmap::Vertex) * this->vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getTransferQueue().queueIndex });
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_VERT_STAGING]);
	}

	// Index generation
	{
		// Index buffer
		std::vector<uint32_t> queueIndices = { Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex };
		uint32_t indexBufferSize = sizeof(unsigned) * this->heightmap.getProximityIndiciesSize();
		this->buffers[BUFFER_INDEX].init(indexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_INDEX]);

		// Config buffer
		this->buffers[BUFFER_CONFIG].init(sizeof(ComputeIndexConfig), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_CONFIG]);
	}
}

void ProjectFinal::setupMemories()
{
	this->memories[MEMORY_HOST_VISIBLE].init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	this->memories[MEMORY_DEVICE_LOCAL].init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//this->memories[MEMORY_TEXTURE].init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); Can not be done here
}

void ProjectFinal::setupDescManagers()
{
	// Graphics
	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		this->descManagers[PIPELINE_GRAPHICS].updateBufferDesc(0, 0, this->buffers[BUFFER_VERTICES].getBuffer(), 0, this->buffers[BUFFER_VERTICES].getSize());
		this->descManagers[PIPELINE_GRAPHICS].updateBufferDesc(0, 1, this->buffers[BUFFER_CAMERA].getBuffer(), 0, this->buffers[BUFFER_CAMERA].getSize());
		this->descManagers[PIPELINE_GRAPHICS].updateSets({ 0 }, i);
	}

	// Frustum compute
	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		this->descManagers[PIPELINE_FRUSTUM].updateBufferDesc(0, 0, this->buffers[BUFFER_INDIRECT_DRAW].getBuffer(), 0, this->buffers[BUFFER_INDIRECT_DRAW].getSize());
		this->descManagers[PIPELINE_FRUSTUM].updateBufferDesc(0, 1, this->buffers[BUFFER_VERTICES].getBuffer(), 0, this->buffers[BUFFER_VERTICES].getSize());
		this->descManagers[PIPELINE_FRUSTUM].updateBufferDesc(0, 2, this->buffers[BUFFER_WORLD_DATA].getBuffer(), 0, this->buffers[BUFFER_WORLD_DATA].getSize());
		this->descManagers[PIPELINE_FRUSTUM].updateBufferDesc(0, 3, this->buffers[BUFFER_PLANES].getBuffer(), 0, this->buffers[BUFFER_PLANES].getSize());
		this->descManagers[PIPELINE_FRUSTUM].updateSets({ 0 }, i);
	}

	// Index compute
	this->descManagers[PIPELINE_INDEX].updateBufferDesc(0, 0, this->buffers[BUFFER_INDEX].getBuffer(), 0, this->buffers[BUFFER_INDEX].getSize());
	this->descManagers[PIPELINE_INDEX].updateBufferDesc(0, 1, this->buffers[BUFFER_CONFIG].getBuffer(), 0, this->buffers[BUFFER_CONFIG].getSize());
	this->descManagers[PIPELINE_INDEX].updateSets({ 0 }, 0);
}

void ProjectFinal::setupCommandBuffers()
{
	this->graphicsPrimary = this->graphicsPools[MAIN_THREAD].createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	this->computePrimary = this->computePools[MAIN_THREAD].createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		for (size_t j = 1; j < FUNC_COUNT_GRAPHICS + 1u; j++)
			this->graphicsSecondary[i].push_back(this->graphicsPools[j % ThreadManager::threadCount()].createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
	}

	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		for (size_t j = 1; j < FUNC_COUNT_COMPUTE + 1u; j++)
			this->computeSecondary[i].push_back(this->computePools[j % ThreadManager::threadCount()].createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
	}
}

void ProjectFinal::setupCommandPools()
{
	// Graphics
	this->graphicsPools.resize(std::min(1u + FUNC_COUNT_GRAPHICS, ThreadManager::threadCount()));
	for (auto& pool : this->graphicsPools)
		pool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	
	// Compute
	this->computePools.resize(std::min(1u + FUNC_COUNT_COMPUTE, ThreadManager::threadCount()));
	for (auto& pool : this->computePools)
		pool.init(CommandPool::Queue::COMPUTE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// Transfer
	this->transferPools.resize(1);
	for (auto& pool : this->transferPools)
		pool.init(CommandPool::Queue::TRANSFER, 0);
}

void ProjectFinal::setupShaders()
{

	// Graphics
	getShader(PIPELINE_GRAPHICS).addStage(Shader::Type::VERTEX, "ComputeTransferTest\\compTransferVert.spv");
	getShader(PIPELINE_GRAPHICS).addStage(Shader::Type::FRAGMENT, "ComputeTransferTest\\compTransferFrag.spv");
	getShader(PIPELINE_GRAPHICS).init();

	// Frustum compute

	getShader(PIPELINE_FRUSTUM).addStage(Shader::Type::COMPUTE, "ComputeTransferTest\\compTransferComp.spv");
	getShader(PIPELINE_FRUSTUM).init();

	// Index compute
	getShader(PIPELINE_INDEX).addStage(Shader::Type::COMPUTE, "ComputeTransferTest\\compTransferIndex.spv");
	getShader(PIPELINE_INDEX).init();
}

void ProjectFinal::setupFrustumPipeline()
{

	getPipeline(PIPELINE_FRUSTUM).setDescriptorLayouts(this->descManagers[PIPELINE_FRUSTUM].getLayouts());
	getPipeline(PIPELINE_FRUSTUM).init(Pipeline::Type::COMPUTE, &getShader(PIPELINE_FRUSTUM));
}

void ProjectFinal::setupIndexPipeline()
{
	getPipeline(PIPELINE_INDEX).setDescriptorLayouts(this->descManagers[PIPELINE_INDEX].getLayouts());
	getPipeline(PIPELINE_INDEX).init(Pipeline::Type::COMPUTE, &getShader(PIPELINE_INDEX));
}

void ProjectFinal::setupGraphicsPipeline()
{
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

	getPipeline(PIPELINE_GRAPHICS).setDescriptorLayouts(this->descManagers[PIPELINE_GRAPHICS].getLayouts());
	getPipeline(PIPELINE_GRAPHICS).setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	//getPipeline(PIPELINE_GRAPHICS).setWireframe(true);
	getPipeline(PIPELINE_GRAPHICS).init(Pipeline::Type::GRAPHICS, &getShader(PIPELINE_GRAPHICS));
}

void ProjectFinal::transferInitalData()
{
	// Set inital indirect draw data
	{
		Buffer stagingBuffer;
		Memory stagingMemory;
		stagingBuffer.init(sizeof(VkDrawIndexedIndirectCommand) * this->regionCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getTransferQueue().queueIndex });
		stagingMemory.bindBuffer(&stagingBuffer);
		stagingMemory.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		std::vector<VkDrawIndexedIndirectCommand> indirectData(this->regionCount);

		for (size_t i = 0; i < indirectData.size(); i++) {
			indirectData[i].instanceCount = 1;
			indirectData[i].firstInstance = i;
		}

		//transferToDevice(&this->buffers[BUFFER_INDIRECT_DRAW], &stagingBuffer, &stagingMemory, indirectData.data(), indirectData.size());

		stagingMemory.directTransfer(&stagingBuffer, indirectData.data(), indirectData.size(), 0);

		CommandBuffer* cbuff = this->graphicsPools[MAIN_THREAD].beginSingleTimeCommand();

		// Copy vertex data.
		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = indirectData.size();
		cbuff->cmdCopyBuffer(stagingBuffer.getBuffer(), this->buffers[BUFFER_INDIRECT_DRAW].getBuffer(), 1, &region);

		cbuff->releaseBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_TRANSFER_READ_BIT, Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

		this->graphicsPools[MAIN_THREAD].endSingleTimeCommand(cbuff);

		stagingBuffer.cleanup();
		stagingMemory.cleanup();
	}

	// Set world data
	{
		WorldData tempData;
		tempData.loadedWidth = this->heightmap.getProximityVertexDim();
		tempData.numIndicesPerReg = this->heightmap.getIndiciesPerRegion();
		tempData.regWidth = this->heightmap.getRegionSize();
		tempData.regionCount = this->heightmap.getProximityRegionCount();
		this->memories[MEMORY_HOST_VISIBLE].directTransfer(&this->buffers[BUFFER_WORLD_DATA], &tempData, sizeof(WorldData), 0);
	}

	// Send inital data to GPU
	verticesToDevice(&this->buffers[BUFFER_VERTICES], this->vertices);
}


void ProjectFinal::transferToDevice(Buffer* buffer, Buffer* stagingBuffer, Memory* stagingMemory, void* data, uint32_t size)
{
	stagingMemory->directTransfer(stagingBuffer, data, size, 0);

	CommandBuffer* cbuff = this->transferPools[MAIN_THREAD].beginSingleTimeCommand();

	// Copy vertex data.
	VkBufferCopy region = {};
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = size;
	cbuff->cmdCopyBuffer(stagingBuffer->getBuffer(), buffer->getBuffer(), 1, &region);

	this->transferPools[MAIN_THREAD].endSingleTimeCommand(cbuff);
}

void ProjectFinal::verticesToDevice(Buffer* buffer, const std::vector<Heightmap::Vertex>& verticies)
{
	transferToDevice(buffer, &this->buffers[BUFFER_VERT_STAGING], &this->memories[MEMORY_HOST_VISIBLE], vertices.data(), vertices.size());
}

void ProjectFinal::secRecordFrustum(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	buffer->begin(0, &inheritanceInfo);
	buffer->cmdBindPipeline(&getPipeline(PIPELINE_FRUSTUM));
	std::vector<VkDescriptorSet> sets = { this->descManagers[PIPELINE_FRUSTUM].getSet(frameIndex, 0) };
	std::vector<uint32_t> offsets;
	buffer->cmdBindDescriptorSets(&getPipeline(PIPELINE_FRUSTUM), 0, sets, offsets);
	buffer->cmdDispatch((uint32_t)ceilf((float)this->regionCount / 16), 1, 1);
	buffer->end();
}

void ProjectFinal::secRecordSkybox(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	this->skybox.draw(buffer, frameIndex);
	buffer->end();
}

void ProjectFinal::secRecordHeightmap(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	buffer->cmdBindPipeline(&getPipeline(PIPELINE_GRAPHICS));
	std::vector<VkDescriptorSet> sets = { this->descManagers[PIPELINE_GRAPHICS].getSet(frameIndex, 0) };
	std::vector<uint32_t> offsets;
	buffer->cmdBindDescriptorSets(&getPipeline(PIPELINE_GRAPHICS), 0, sets, offsets);
	buffer->cmdBindIndexBuffer(this->buffers[BUFFER_INDEX].getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	buffer->cmdDrawIndexedIndirect(this->buffers[BUFFER_INDIRECT_DRAW].getBuffer(), 0, this->regionCount, sizeof(VkDrawIndexedIndirectCommand));
	buffer->end();
}

void ProjectFinal::record(uint32_t frameIndex)
{
	uint32_t threadIndex = 0;
	uint32_t secondaryBuffer = 0;
	auto nextThread = [&threadIndex]() -> uint32_t {
		uint32_t id = threadIndex;
		threadIndex = ++threadIndex % ThreadManager::threadCount();
		return id;
	};

	VkCommandBufferInheritanceInfo inheritInfo = {};
	inheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	CommandBuffer* buffer;

	// Frustum compute
	buffer = this->computeSecondary[frameIndex][secondaryBuffer++];
	ThreadManager::addWork(nextThread(), [=]() { secRecordFrustum(frameIndex, buffer, inheritInfo); });

	// Graphics
	secondaryBuffer = 0;
	inheritInfo.framebuffer = getFramebuffers()[frameIndex].getFramebuffer();
	inheritInfo.renderPass = this->renderPass.getRenderPass();

	// Skybox
	buffer = this->graphicsSecondary[frameIndex][secondaryBuffer++];
	ThreadManager::addWork(nextThread(), [=]() { secRecordSkybox(frameIndex, buffer, inheritInfo); });

	// Heightmap
	buffer = this->graphicsSecondary[frameIndex][secondaryBuffer++];
	ThreadManager::addWork(nextThread(), [=]() { secRecordHeightmap(frameIndex, buffer, inheritInfo); });


	// Primary recording
	// Graphics
	buffer = this->graphicsPrimary[frameIndex];
	buffer->begin(0, nullptr);

	buffer->acquireBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
		Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	buffer->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[frameIndex].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	std::vector<VkCommandBuffer> vkCommands;
	for (size_t i = 0; i < this->graphicsSecondary[frameIndex].size(); i++)
		vkCommands.push_back(this->graphicsSecondary[frameIndex][i]->getCommandBuffer());
	ThreadManager::wait();
	buffer->cmdExecuteCommands(vkCommands.size(), vkCommands.data());
	buffer->cmdEndRenderPass();

	buffer->releaseBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
		Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	buffer->end();

	// Compute
	buffer = this->computePrimary[frameIndex];
	buffer->begin(0, nullptr);

	buffer->acquireBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	vkCommands.clear();
	for (size_t i = 0; i < this->computeSecondary[frameIndex].size(); i++)
		vkCommands.push_back(this->computeSecondary[frameIndex][i]->getCommandBuffer());
	buffer->cmdExecuteCommands(vkCommands.size(), vkCommands.data());

	buffer->releaseBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_SHADER_WRITE_BIT,
		Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	buffer->end();
}
