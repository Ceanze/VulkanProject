#include "jaspch.h"
#include "ProjectFinalNaive.h"

#include <stb/stb_image.h>
#include <GLFW/glfw3.h>

#include "Core/Camera.h"
#include "Threading/ThreadDispatcher.h"
#include "Threading/ThreadManager.h"
#include "Vulkan/VulkanProfiler.h"
#include "Core/CPUProfiler.h"
#include "Models/GLTFLoader.h"
#include "Models/ModelRenderer.h"

#define MAIN_THREAD 0

void ProjectFinalNaive::init()
{
	getFrame()->queueUsage(VK_QUEUE_GRAPHICS_BIT);

	// Initalize with maximum available threads
	ThreadDispatcher::init(2);
	ThreadManager::init(static_cast<uint32_t>(std::thread::hardware_concurrency()));
	setupCommandPools();

	setupModels();
	setupHeightmap();
	setupDescLayouts();
	setupGeneral();
	setupBuffers();
	setupMemories();
	setupDescManagers();
	setupCommandBuffers();

	VulkanProfiler::get().init(&this->graphicsPools[MAIN_THREAD], 10, 60, VulkanProfiler::TimeUnit::MICRO);
	VulkanProfiler::get().createTimestamps(6 + (FUNC_COUNT_COMPUTE + FUNC_COUNT_GRAPHICS) * 3);
	VulkanProfiler::get().addIndexedTimestamps("Graphics", 3, this->graphicsPrimary.data());
	VulkanProfiler::get().addIndexedTimestamps("Models", 3, this->graphicsPrimary.data());
	VulkanProfiler::get().addIndexedTimestamps("Skybox", 3, this->graphicsPrimary.data());
	VulkanProfiler::get().addIndexedTimestamps("Heightmap", 3, this->graphicsPrimary.data());
	VulkanProfiler::get().addIndexedTimestamps("Frustum", 3, this->graphicsPrimary.data());

	transferInitalData();
}

void ProjectFinalNaive::loop(float dt)
{
	JAS_PROFILER_TOGGLE_SAMPLE_POOL(&this->graphicsPools[MAIN_THREAD], GLFW_KEY_R, 10);
	JAS_PROFILER_SAMPLE_FUNCTION();

	// Update view matrix
	{
		JAS_PROFILER_SAMPLE_SCOPE("Update camera");
		this->camera->update(dt, this->heightmap.getTerrainHeight(this->camera->getPosition().x, this->camera->getPosition().z));
	}


	// Transfer vertex data when proximity changes
	transferVertexData();

	// Render
	getFrame()->beginFrame(dt);
	record(getFrame()->getCurrentImageIndex());
	//getFrame()->submitCompute(Instance::get().getComputeQueue().queue, this->computePrimary[getFrame()->getCurrentImageIndex()]);
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->graphicsPrimary.data());
	getFrame()->endFrame();
}

void ProjectFinalNaive::cleanup()
{
	ThreadDispatcher::shutdown();
	ThreadManager::cleanup();
	VulkanProfiler::get().cleanup();

	GLTFLoader::cleanupDefaultData();

	for (auto& descManager : this->descManagers)
		descManager.second.cleanup();

	for (auto& model : this->models)
		model.second.cleanup();

	for (auto& pool : this->graphicsPools)
		pool.cleanup();

	this->graphicsTransferPool.cleanup();

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

void ProjectFinalNaive::setupHeightmap()
{
	this->regionSize = 8;

	float scale = 2.0f;
	this->heightmap.setVertexDist(scale);
	this->heightmap.setProximitySize(20);
	this->heightmap.setMaxZ(20.f);
	this->heightmap.setMinZ(0.f);

	int width, height;
	int channels;
	std::string path = "../assets/Textures/ireland.jpg";
	unsigned char* data = static_cast<unsigned char*>(stbi_load(path.c_str(), &width, &height, &channels, 1));
	if (data == nullptr)
		JAS_ERROR("Failed to load heightmap, couldn't find file!");
	else {
		JAS_INFO("Loaded heightmap: {} successfully!", path);
		this->heightmap.init({ -(width / 2.f) * scale, 0.f, -(height / 2.f) * scale }, this->regionSize, width, height, data);
		JAS_INFO("Initilized heightmap successfully!");
		delete[] data;
	}

	float th = this->heightmap.getTerrainHeight(0, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, th, 0.f }, { 0.f, th, 1.f }, 10.0f, 10.0f, true);

	// Set data used for transfer
	this->lastRegionIndex = this->heightmap.getRegionFromPos(this->camera->getPosition());
	this->transferThreshold = 1;

	this->regionCount = this->heightmap.getProximityRegionCount();

	this->vertices.resize(this->heightmap.getProximityVertexDim() * this->heightmap.getProximityVertexDim());
	this->heightmap.getProximityVerticies(this->camera->getPosition(), this->vertices);
}

void ProjectFinalNaive::setupModels()
{
	GLTFLoader::initDefaultData(&this->graphicsPools[MAIN_THREAD]);

	this->treeCount = 100000;
	// This can now be done in another thread.
	const std::string filePath = "..\\assets\\Models\\Tree\\tree.glb";
	//const std::string filePath = "..\\assets\\Models\\Sponza\\glTF\\Sponza.gltf";

	GLTFLoader::StagingBuffers stagingBuffers;
	GLTFLoader::prepareStagingBuffer(filePath, &this->models[MODEL_TREE], &stagingBuffers);
	stagingBuffers.initMemory();
	GLTFLoader::transferToModel(&this->graphicsPools[MAIN_THREAD], &this->models[MODEL_TREE], &stagingBuffers);
	stagingBuffers.cleanup();

	//this->isTransferDone = false;
	//this->thread = new std::thread(&TransferTest::loadingThread, this);

	ModelRenderer::get().init();
}

void ProjectFinalNaive::setupDescLayouts()
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

	// Models: Set 0
	{
		DescriptorLayout descLayout;
		descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
		descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Transforms
		descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // World Vp
		descLayout.init();
		this->descManagers[PIPELINE_MODELS].addLayout(descLayout);
		std::vector<DescriptorLayout> descLayouts(this->models[MODEL_TREE].materials.size());
		for (uint32_t i = 0; i < (uint32_t)this->models[MODEL_TREE].materials.size(); i++)
		{
			Material& material = this->models[MODEL_TREE].materials[i];
			Material::initializeDescriptor(descLayouts[i]);
			this->descManagers[PIPELINE_MODELS].addLayout(descLayouts[i]);
		}
		this->descManagers[PIPELINE_MODELS].init(getSwapChain()->getNumImages());
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

void ProjectFinalNaive::setupGeneral()
{
	getShaders().resize(PIPELINE_COUNT);
	getPipelines().resize(PIPELINE_COUNT);

	setupShaders();
	setupGraphicsPipeline();
	setupModelsPipeline();
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

void ProjectFinalNaive::setupBuffers()
{
	// Graphics buffers
	{
		std::vector<uint32_t> queueIndices = { Instance::get().getGraphicsQueue().queueIndex };
		this->buffers[BUFFER_CAMERA].init(sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->buffers[BUFFER_MODEL_TRANSFORMS].init(sizeof(glm::mat4) * this->treeCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_CAMERA]);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_MODEL_TRANSFORMS]);
	}

	// Frustum buffers
	{
		// Planes and world data
		std::vector<uint32_t> queueIndices = { Instance::get().getGraphicsQueue().queueIndex };
		this->buffers[BUFFER_PLANES].init(sizeof(Camera::Plane) * 6, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->buffers[BUFFER_WORLD_DATA].init(sizeof(WorldData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_PLANES]);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_WORLD_DATA]);

		// Indirect draw data
		this->buffers[BUFFER_INDIRECT_DRAW].init(sizeof(VkDrawIndexedIndirectCommand) * this->regionCount, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			{ Instance::get().getGraphicsQueue().queueIndex });
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_INDIRECT_DRAW]);

		// Compute vertex data
		this->buffers[BUFFER_VERTICES].init(sizeof(Heightmap::Vertex) * this->vertices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->buffers[BUFFER_VERTICES_2].init(sizeof(Heightmap::Vertex) * this->vertices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_VERTICES]);
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_VERTICES_2]);

		// Vert staging
		this->buffers[BUFFER_VERT_STAGING].init(sizeof(Heightmap::Vertex) * this->vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getGraphicsQueue().queueIndex });
		this->memories[MEMORY_VERT_STAGING].bindBuffer(&this->buffers[BUFFER_VERT_STAGING]);
	}

	// Index generation
	{
		// Index buffer
		std::vector<uint32_t> queueIndices = { Instance::get().getGraphicsQueue().queueIndex };
		uint32_t indexBufferSize = sizeof(unsigned) * this->heightmap.getProximityIndiciesSize();
		this->buffers[BUFFER_INDEX].init(indexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_DEVICE_LOCAL].bindBuffer(&this->buffers[BUFFER_INDEX]);

		// Config buffer
		this->buffers[BUFFER_CONFIG].init(sizeof(ComputeIndexConfig), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
		this->memories[MEMORY_HOST_VISIBLE].bindBuffer(&this->buffers[BUFFER_CONFIG]);
	}
}

void ProjectFinalNaive::setupMemories()
{
	this->memories[MEMORY_HOST_VISIBLE].init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	this->memories[MEMORY_DEVICE_LOCAL].init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	this->memories[MEMORY_VERT_STAGING].init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	//this->memories[MEMORY_TEXTURE].init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); Can not be done here
}

void ProjectFinalNaive::setupDescManagers()
{
	// Graphics
	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		this->descManagers[PIPELINE_GRAPHICS].updateBufferDesc(0, 0, this->buffers[BUFFER_VERTICES].getBuffer(), 0, this->buffers[BUFFER_VERTICES].getSize());
		this->descManagers[PIPELINE_GRAPHICS].updateBufferDesc(0, 1, this->buffers[BUFFER_CAMERA].getBuffer(), 0, this->buffers[BUFFER_CAMERA].getSize());
		this->descManagers[PIPELINE_GRAPHICS].updateSets({ 0 }, i);
	}

	// Models
	for (uint32_t i = 0; i < static_cast<uint32_t>(getSwapChain()->getNumImages()); i++)
	{
		VkDeviceSize vertexBufferSize = this->models[MODEL_TREE].vertices.size() * sizeof(Vertex);
		this->descManagers[PIPELINE_MODELS].updateBufferDesc(0, 0, this->models[MODEL_TREE].vertexBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManagers[PIPELINE_MODELS].updateBufferDesc(0, 1, this->buffers[BUFFER_MODEL_TRANSFORMS].getBuffer(), 0, this->buffers[BUFFER_MODEL_TRANSFORMS].getSize());
		this->descManagers[PIPELINE_MODELS].updateBufferDesc(0, 2, this->buffers[BUFFER_CAMERA].getBuffer(), 0, sizeof(CameraData));
		auto& materials = this->models[MODEL_TREE].materials;
		std::vector<uint32_t> sets(1 + materials.size(), 0);
		for (uint32_t j = 1; j <= (uint32_t)materials.size(); j++)
		{
			Material& material = materials[j - 1];
			sets[j] = j;
			this->descManagers[PIPELINE_MODELS].updateImageDesc(j, 0, material.baseColorTexture.texture->getImage().getLayout(),
				material.baseColorTexture.texture->getVkImageView(), material.baseColorTexture.sampler->getSampler());

			this->descManagers[PIPELINE_MODELS].updateImageDesc(j, 1, material.metallicRoughnessTexture.texture->getImage().getLayout(),
				material.metallicRoughnessTexture.texture->getVkImageView(), material.metallicRoughnessTexture.sampler->getSampler());

			this->descManagers[PIPELINE_MODELS].updateImageDesc(j, 2, material.normalTexture.texture->getImage().getLayout(),
				material.normalTexture.texture->getVkImageView(), material.normalTexture.sampler->getSampler());

			this->descManagers[PIPELINE_MODELS].updateImageDesc(j, 3, material.occlusionTexture.texture->getImage().getLayout(),
				material.occlusionTexture.texture->getVkImageView(), material.occlusionTexture.sampler->getSampler());

			this->descManagers[PIPELINE_MODELS].updateImageDesc(j, 4, material.emissiveTexture.texture->getImage().getLayout(),
				material.emissiveTexture.texture->getVkImageView(), material.emissiveTexture.sampler->getSampler());
		}
		this->descManagers[PIPELINE_MODELS].updateSets(sets, i);
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

void ProjectFinalNaive::setupCommandBuffers()
{
	this->graphicsPrimary = this->graphicsPools[MAIN_THREAD].createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	//this->computePrimary = this->computePools[MAIN_THREAD].createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		for (size_t j = 1; j < FUNC_COUNT_GRAPHICS + 1u; j++)
			this->graphicsSecondary[i].push_back(this->graphicsPools[j % ThreadManager::threadCount()].createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
	}

	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++) {
		for (size_t j = 0; j < FUNC_COUNT_COMPUTE; j++)
			this->computeSecondary[i].push_back(this->graphicsPools[(j + FUNC_COUNT_GRAPHICS + 1u) % ThreadManager::threadCount()].createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
	}
	this->transferBuffer = this->graphicsTransferPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
}

void ProjectFinalNaive::setupCommandPools()
{
	// Graphics
	this->graphicsPools.resize(std::min(1u + FUNC_COUNT_GRAPHICS + FUNC_COUNT_COMPUTE, ThreadManager::threadCount()));
	for (auto& pool : this->graphicsPools)
		pool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	this->graphicsTransferPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// Compute
	//this->computePools.resize(std::min(1u + FUNC_COUNT_COMPUTE, ThreadManager::threadCount()));
	//for (auto& pool : this->computePools)
	//	pool.init(CommandPool::Queue::COMPUTE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	//// Transfer
	//this->transferPools.resize(1);
	//for (auto& pool : this->transferPools)
	//	pool.init(CommandPool::Queue::TRANSFER, 0);
}

void ProjectFinalNaive::setupShaders()
{
	// Graphics
	getShader(PIPELINE_GRAPHICS).addStage(Shader::Type::VERTEX, "ComputeTransferTest\\compTransferVert.spv");
	getShader(PIPELINE_GRAPHICS).addStage(Shader::Type::FRAGMENT, "ComputeTransferTest\\compTransferFrag.spv");
	getShader(PIPELINE_GRAPHICS).init();

	// Model
	getShader(PIPELINE_MODELS).addStage(Shader::Type::VERTEX, "modelVertex.spv");
	getShader(PIPELINE_MODELS).addStage(Shader::Type::FRAGMENT, "modelFragment.spv");
	getShader(PIPELINE_MODELS).init();

	// Frustum compute
	getShader(PIPELINE_FRUSTUM).addStage(Shader::Type::COMPUTE, "ComputeTransferTest\\compTransferComp.spv");
	getShader(PIPELINE_FRUSTUM).init();

	// Index compute
	getShader(PIPELINE_INDEX).addStage(Shader::Type::COMPUTE, "ComputeTransferTest\\compTransferIndex.spv");
	getShader(PIPELINE_INDEX).init();
}

void ProjectFinalNaive::setupFrustumPipeline()
{

	getPipeline(PIPELINE_FRUSTUM).setDescriptorLayouts(this->descManagers[PIPELINE_FRUSTUM].getLayouts());
	getPipeline(PIPELINE_FRUSTUM).init(Pipeline::Type::COMPUTE, &getShader(PIPELINE_FRUSTUM));
}

void ProjectFinalNaive::setupIndexPipeline()
{
	getPipeline(PIPELINE_INDEX).setDescriptorLayouts(this->descManagers[PIPELINE_INDEX].getLayouts());
	getPipeline(PIPELINE_INDEX).init(Pipeline::Type::COMPUTE, &getShader(PIPELINE_INDEX));
}

void ProjectFinalNaive::setupGraphicsPipeline()
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

void ProjectFinalNaive::setupModelsPipeline()
{
	PushConstants& pushConstants = ModelRenderer::get().getPushConstants();
	getPipeline(PIPELINE_MODELS).setPushConstants(pushConstants);
	getPipeline(PIPELINE_MODELS).setDescriptorLayouts(this->descManagers[PIPELINE_MODELS].getLayouts());
	getPipeline(PIPELINE_MODELS).setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	getPipeline(PIPELINE_MODELS).init(Pipeline::Type::GRAPHICS, &getShader(PIPELINE_MODELS));
}

void ProjectFinalNaive::transferInitalData()
{
	// Set inital indirect draw data
	{
		Buffer stagingBuffer;
		Memory stagingMemory;
		stagingBuffer.init(sizeof(VkDrawIndexedIndirectCommand) * this->regionCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { Instance::get().getGraphicsQueue().queueIndex });
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

	/*	cbuff->releaseBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_TRANSFER_READ_BIT, Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);*/

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

	// Set model transform data
	{
		std::srand((unsigned)std::time(0));
		auto rnd11 = [](int precision = 10000) { return (float)(std::rand() % precision) / (float)precision; };
		auto rnd = [rnd11](float min, float max) { return min + rnd11(RAND_MAX) * glm::abs(max - min); };

		std::vector<glm::mat4> matrices(this->treeCount);
		for (uint32_t i = 0; i < this->treeCount; i++)
		{
			float w = (float)this->heightmap.getWidth() * this->heightmap.getVertexDist();
			float a = -w / 2;
			float b = w / 2;
			glm::vec3 pos(rnd(a, b), 0.f, rnd(a, b));
			pos.y = this->heightmap.getTerrainHeight(pos.x, pos.z);
			matrices[i] = glm::translate(glm::mat4(1.0), pos);
		}
		this->memories[MEMORY_HOST_VISIBLE].directTransfer(&this->buffers[BUFFER_MODEL_TRANSFORMS], matrices.data(), this->buffers[BUFFER_MODEL_TRANSFORMS].getSize(), 0);
	}

	// Send inital data to GPU
	verticesToDevice(&this->buffers[BUFFER_VERTICES], this->vertices);
	this->compVertInactiveBuffer = &this->buffers[BUFFER_VERTICES_2];

	// Submit generate indicies work to GPU once
	{
		ComputeIndexConfig cfg;
		cfg.regionCount = this->heightmap.getProximityWidthRegionCount();
		cfg.regionSize = this->regionSize;
		cfg.proximitySize = this->heightmap.getProximityVertexDim();
		cfg.pad2 = 0;
		this->memories[MEMORY_HOST_VISIBLE].directTransfer(&this->buffers[BUFFER_CONFIG], &cfg, sizeof(ComputeIndexConfig), 0);

		CommandBuffer* cmdBuff = this->graphicsPools[MAIN_THREAD].createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		cmdBuff->begin(0, nullptr);

		cmdBuff->cmdBindPipeline(&getPipeline(PIPELINE_INDEX));
		std::vector<VkDescriptorSet> sets = { this->descManagers[PIPELINE_INDEX].getSet({ 0 }, 0) };
		std::vector<uint32_t> offsets;
		cmdBuff->cmdBindDescriptorSets(&getPipeline(PIPELINE_INDEX), 0, sets, offsets);

		// Dispatch the compute job
		// The compute shader will do the frustum culling and adjust the indirect draw calls depending on object visibility.
		cmdBuff->cmdDispatch((uint32_t)ceilf((float)this->heightmap.getProximityRegionCount() / 16), 1, 1);

		cmdBuff->end();

		VkQueue graphQueue = Instance::get().getGraphicsQueue().queue;
		VkSubmitInfo computeSubmitInfo = { };
		std::array<VkCommandBuffer, 1> buff = { cmdBuff->getCommandBuffer() };
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.commandBufferCount = buff.size();
		computeSubmitInfo.pCommandBuffers = buff.data();
		computeSubmitInfo.signalSemaphoreCount = 0;
		computeSubmitInfo.pSignalSemaphores = nullptr;

		ERROR_CHECK(vkQueueSubmit(graphQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE), "Failed to submit compute queue!");
	}
}

void ProjectFinalNaive::transferVertexData()
{
	glm::vec3 camPos = this->camera->getPosition();
	glm::ivec2 currRegion = this->heightmap.getRegionFromPos(camPos);
	glm::ivec2 diff = this->lastRegionIndex - currRegion;
	if (abs(diff.x) > this->transferThreshold || abs(diff.y) > this->transferThreshold) {
		if (ThreadDispatcher::finished()) {
			this->lastRegionIndex = currRegion;
			// Transfer proximity verticies to device
			uint32_t id = ThreadDispatcher::dispatch([&, camPos]() {
				this->heightmap.getProximityVerticies(camPos, this->vertices);
				verticesToDevice(this->compVertInactiveBuffer, this->vertices);
			});

			this->workIds.push(id);
		}
	}

	if (!this->workIds.empty())
	{
		uint32_t id = this->workIds.front();
		if (ThreadDispatcher::finished(id))
		{
			Buffer* activeBuffer = nullptr;
			if (this->compVertInactiveBuffer == &this->buffers[BUFFER_VERTICES_2])
			{
				this->compVertInactiveBuffer = &this->buffers[BUFFER_VERTICES];
				activeBuffer = &this->buffers[BUFFER_VERTICES_2];
			}
			else
			{
				this->compVertInactiveBuffer = &this->buffers[BUFFER_VERTICES_2];
				activeBuffer = &this->buffers[BUFFER_VERTICES];
			}

			vkQueueWaitIdle(Instance::get().getGraphicsQueue().queue);
			//vkQueueWaitIdle(Instance::get().getComputeQueue().queue);
			VkDeviceSize vertexBufferSize = this->vertices.size() * sizeof(Heightmap::Vertex);
			for (uint32_t i = 0; i < static_cast<uint32_t>(getSwapChain()->getNumImages()); i++)
			{
				this->descManagers[PIPELINE_GRAPHICS].updateBufferDesc(0, 0, activeBuffer->getBuffer(), 0, vertexBufferSize);
				this->descManagers[PIPELINE_GRAPHICS].updateSets({ 0 }, i);

				this->descManagers[PIPELINE_FRUSTUM].updateBufferDesc(0, 1, activeBuffer->getBuffer(), 0, vertexBufferSize);
				this->descManagers[PIPELINE_FRUSTUM].updateSets({ 0 }, i);
			}

			this->workIds.pop();
		}
	}
}


void ProjectFinalNaive::transferToDevice(Buffer* buffer, Buffer* stagingBuffer, Memory* stagingMemory, void* data, uint32_t size)
{
	stagingMemory->directTransfer(stagingBuffer, data, size, 0);

	CommandBuffer* cbuff = this->transferBuffer;
	VkCommandBufferInheritanceInfo inheritInfo = {};
	inheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	cbuff->begin(0, &inheritInfo);

	// Copy vertex data.
	VkBufferCopy region = {};
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = size;
	cbuff->cmdCopyBuffer(stagingBuffer->getBuffer(), buffer->getBuffer(), 1, &region);

	cbuff->end();
	transferUpdated = true;
}

void ProjectFinalNaive::verticesToDevice(Buffer* buffer, const std::vector<Heightmap::Vertex>& verticies)
{
	transferToDevice(buffer, &this->buffers[BUFFER_VERT_STAGING], &this->memories[MEMORY_VERT_STAGING], vertices.data(), vertices.size() * sizeof(Heightmap::Vertex));
}

void ProjectFinalNaive::secRecordFrustum(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	buffer->begin(0, &inheritanceInfo);
	VulkanProfiler::get().startIndexedTimestamp("Frustum", buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameIndex);
	std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(1)));
	buffer->cmdBindPipeline(&getPipeline(PIPELINE_FRUSTUM));
	std::vector<VkDescriptorSet> sets = { this->descManagers[PIPELINE_FRUSTUM].getSet(frameIndex, 0) };
	std::vector<uint32_t> offsets;
	buffer->cmdBindDescriptorSets(&getPipeline(PIPELINE_FRUSTUM), 0, sets, offsets);
	buffer->cmdDispatch((uint32_t)ceilf((float)this->regionCount / 16), 1, 1);
	VulkanProfiler::get().endIndexedTimestamp("Frustum", buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameIndex);
	buffer->end();
}

void ProjectFinalNaive::secRecordSkybox(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	VulkanProfiler::get().startIndexedTimestamp("Skybox", buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameIndex);
	std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(1)));
	this->skybox.draw(buffer, frameIndex);
	VulkanProfiler::get().endIndexedTimestamp("Skybox", buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameIndex);
	buffer->end();
}

void ProjectFinalNaive::secRecordHeightmap(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	VulkanProfiler::get().startIndexedTimestamp("Heightmap", buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameIndex);
	std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(1)));
	buffer->cmdBindPipeline(&getPipeline(PIPELINE_GRAPHICS));
	std::vector<VkDescriptorSet> sets = { this->descManagers[PIPELINE_GRAPHICS].getSet(frameIndex, 0) };
	std::vector<uint32_t> offsets;
	buffer->cmdBindDescriptorSets(&getPipeline(PIPELINE_GRAPHICS), 0, sets, offsets);
	buffer->cmdBindIndexBuffer(this->buffers[BUFFER_INDEX].getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	buffer->cmdDrawIndexedIndirect(this->buffers[BUFFER_INDIRECT_DRAW].getBuffer(), 0, this->regionCount, sizeof(VkDrawIndexedIndirectCommand));
	VulkanProfiler::get().endIndexedTimestamp("Heightmap", buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameIndex);
	buffer->end();
}

void ProjectFinalNaive::secRecordModels(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	VulkanProfiler::get().startIndexedTimestamp("Models", buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameIndex);
	std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(1)));
	buffer->cmdBindPipeline(&getPipeline(PIPELINE_MODELS));
	std::vector<uint32_t> offsets;
	std::vector<VkDescriptorSet> sets = { this->descManagers[PIPELINE_MODELS].getSet(frameIndex, 0) };
	for (Material& material : this->models[MODEL_TREE].materials)
		sets.push_back(this->descManagers[PIPELINE_MODELS].getSet(frameIndex, material.index + 1));
	ModelRenderer::get().record(&this->models[MODEL_TREE], glm::mat4(1.0), buffer, &getPipeline(PIPELINE_MODELS), sets, offsets, this->treeCount);
	VulkanProfiler::get().endIndexedTimestamp("Models", buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameIndex);
	buffer->end();
}

void ProjectFinalNaive::record(uint32_t frameIndex)
{
	JAS_PROFILER_SAMPLE_SCOPE("Record " + std::to_string(frameIndex));
	{
		CommandBuffer* buffer = this->graphicsPrimary[frameIndex];
		VulkanProfiler::get().getBufferTimestamps(buffer);
	}

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

	const int numWork = 1;

	// Frustum compute
	buffer = this->computeSecondary[frameIndex][secondaryBuffer++];
	for (int i = 0; i < numWork; i++)
		secRecordFrustum(frameIndex, buffer, inheritInfo);

	// Graphics
	secondaryBuffer = 0;
	inheritInfo.framebuffer = getFramebuffers()[frameIndex].getFramebuffer();
	inheritInfo.renderPass = this->renderPass.getRenderPass();

	// Skybox
	buffer = this->graphicsSecondary[frameIndex][secondaryBuffer++];
	for (int i = 0; i < numWork; i++)
		secRecordSkybox(frameIndex, buffer, inheritInfo);

	// Heightmap
	buffer = this->graphicsSecondary[frameIndex][secondaryBuffer++];
	for (int i = 0; i < numWork; i++)
		secRecordHeightmap(frameIndex, buffer, inheritInfo);

	// Models
	buffer = this->graphicsSecondary[frameIndex][secondaryBuffer++];
	for (int i = 0; i < numWork; i++)
		secRecordModels(frameIndex, buffer, inheritInfo);

	// Primary recording
	// Graphics
	buffer = this->graphicsPrimary[frameIndex];
	buffer->begin(0, nullptr);
	VulkanProfiler::get().resetBufferTimestamps(buffer);
	VulkanProfiler::get().startIndexedTimestamp("Graphics", buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameIndex);

	Skybox::CubemapUboData cubemapUboData;
	cubemapUboData.proj = camera->getProjection();
	cubemapUboData.view = camera->getView();
	// Disable translation
	cubemapUboData.view[3][0] = 0.0f;
	cubemapUboData.view[3][1] = 0.0f;
	cubemapUboData.view[3][2] = 0.0f;

	vkCmdUpdateBuffer(buffer->getCommandBuffer(), this->skybox.getBuffer()->getBuffer(), 0, this->skybox.getBuffer()->getSize(), (void*)&cubemapUboData);
	vkCmdUpdateBuffer(buffer->getCommandBuffer(), this->buffers[BUFFER_CAMERA].getBuffer(), 0, this->buffers[BUFFER_CAMERA].getSize(), (void*)&this->camera->getMatrix()[0]);
	vkCmdUpdateBuffer(buffer->getCommandBuffer(), this->buffers[BUFFER_PLANES].getBuffer(), 0, this->buffers[BUFFER_PLANES].getSize(), (void*)&this->camera->getPlanes()[0]);
	//buffer->acquireBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
	//	Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex,
	//	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);

	// Compute
	std::vector<VkCommandBuffer> vkCommands;
	for (size_t i = 0; i < this->computeSecondary[frameIndex].size(); i++)
		vkCommands.push_back(this->computeSecondary[frameIndex][i]->getCommandBuffer());

	if (transferUpdated) {
		vkCommands.push_back(this->transferBuffer->getCommandBuffer());
		transferUpdated = false;
	}
	
	buffer->cmdExecuteCommands(vkCommands.size(), vkCommands.data());

	// Graphics
	buffer->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[frameIndex].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	vkCommands.clear();
	for (size_t i = 0; i < this->graphicsSecondary[frameIndex].size(); i++)
		vkCommands.push_back(this->graphicsSecondary[frameIndex][i]->getCommandBuffer());
	
	buffer->cmdExecuteCommands(vkCommands.size(), vkCommands.data());

	buffer->cmdEndRenderPass();
	VulkanProfiler::get().endIndexedTimestamp("Graphics", buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameIndex);

	//buffer->releaseBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
	//	VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	//	VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);


	buffer->end();

	// Compute
	//buffer = this->computePrimary[frameIndex];
	//buffer->begin(0, nullptr);

	//buffer->acquireBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
	//	Instance::get().getGraphicsQueue().queueIndex, Instance::get().getComputeQueue().queueIndex,
	//	VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	//vkCommands.clear();
	//for (size_t i = 0; i < this->computeSecondary[frameIndex].size(); i++)
	//	vkCommands.push_back(this->computeSecondary[frameIndex][i]->getCommandBuffer());
	//buffer->cmdExecuteCommands(vkCommands.size(), vkCommands.data());

	//buffer->releaseBuffer(&this->buffers[BUFFER_INDIRECT_DRAW], VK_ACCESS_SHADER_WRITE_BIT,
	//	Instance::get().getComputeQueue().queueIndex, Instance::get().getGraphicsQueue().queueIndex,
	//	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	//buffer->end();
}