#pragma once
#include "VKSandboxBase.h"
#include "jaspch.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Core/Skybox.h"
#include "Core/Heightmap/Heightmap.h"
#include "Vulkan/Texture.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Pipeline/RenderPass.h"

class Camera;

typedef uint32_t PrimaryIndex;

class ProjectFinalNaive : public VKSandboxBase
{
private:
	enum BufferID {
		BUFFER_PLANES,
		BUFFER_PLANES_STAGE,
		BUFFER_WORLD_DATA,
		BUFFER_MODEL_TRANSFORMS,
		BUFFER_INDIRECT_DRAW,
		BUFFER_VERTICES,
		BUFFER_VERTICES_2,
		BUFFER_VERT_STAGING,
		BUFFER_CAMERA,
		BUFFER_CAMERA_STAGE,
		BUFFER_INDEX,
		BUFFER_CONFIG
	};

	enum MemoryType {
		MEMORY_DEVICE_LOCAL,
		MEMORY_HOST_VISIBLE,
		MEMORY_VERT_STAGING,
		MEMORY_TEXTURE
	};

	enum PipelineID {
		PIPELINE_GRAPHICS = 0,
		PIPELINE_MODELS,
		PIPELINE_FRUSTUM,
		PIPELINE_INDEX,
		PIPELINE_COUNT
	};

	enum WorkFunctionGraphics {
		FUNC_HEIGHTMAP = 0,
		FUNC_MODELS,
		FUNC_SKYBOX,
		FUNC_COUNT_GRAPHICS
	};

	enum WorkFunctionCompute {
		FUNC_FRUSTUM = 0,
		FUNC_COUNT_COMPUTE
	};

	enum WorkFunctionTransfer {
		FUNC_TRANSFER_CAMERA = 0,
		FUNC_TRANSFER_PLANES,
		FUNC_COUNT_TRANSFER
	};

	enum ModelID {
		MODEL_TREE = 0,
		MODEL_COUNT
	};

	struct ComputeIndexConfig {
		uint32_t regionCount;  // Number of regions in width for proximity
		uint32_t regionSize;   // Number of vertices in width for one region.
		uint32_t proximitySize;
		uint32_t pad2;
	};

	struct CameraData
	{
		glm::mat4 vp;
	};

	struct WorldData
	{
		uint32_t regWidth;           // Region width in number of vertices.
		uint32_t numIndicesPerReg;   // Number of indices per region.
		uint32_t loadedWidth;        // Loaded world width in verticies
		uint32_t regionCount;
	};

public:
	virtual void init() override;
	virtual void loop(float dt) override;
	virtual void cleanup() override;

private:
	void setupHeightmap();

	void setupSyncObjects();
	void setupModels();
	void setupDescLayouts();
	void setupGeneral();
	void setupBuffers();
	void setupMemories();
	void setupDescManagers();
	void setupCommandBuffers();

	void setupCommandPools();
	void setupShaders();
	void setupFrustumPipeline();
	void setupIndexPipeline();
	void setupGraphicsPipeline();
	void setupModelsPipeline();

	void transferInitalData();
	void transferVertexData();

	void transferToDevice(Buffer* buffer, Buffer* stagingBuffer, Memory* stagingMemory, void* data, uint32_t size);
	void verticesToDevice(Buffer* buffer, const std::vector<Heightmap::Vertex>& verticies);

	void secRecordTransfer(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo, Buffer& stage, Buffer& Device, void* data);

	void secRecordFrustum(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo);
	void secRecordSkybox(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo);
	void secRecordHeightmap(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo);
	void secRecordModels(uint32_t frameIndex, CommandBuffer* buffer, VkCommandBufferInheritanceInfo inheritanceInfo);

	void record(uint32_t frameIndex);

	void updateDescManagers();

private:
	uint32_t treeCount;
	std::unordered_map<ModelID, Model> models;

	std::unordered_map<BufferID, Buffer> buffers;
	std::unordered_map<MemoryType, Memory> memories;
	std::unordered_map<PipelineID, DescriptorManager> descManagers;

	std::vector<CommandPool> graphicsPools;
	// Pool used for transferring vertices
	CommandPool graphicsTransferPool;

	bool transferUpdated = false;
	CommandBuffer* transferBuffer;
	std::vector<CommandBuffer*> graphicsPrimary;
	std::unordered_map<PrimaryIndex, std::vector<CommandBuffer*>> graphicsSecondary;
	std::unordered_map<PrimaryIndex, std::vector<CommandBuffer*>> computeSecondary;
	std::unordered_map<PrimaryIndex, std::vector<CommandBuffer*>> transferSecondary;

	Skybox skybox;
	Camera* camera;

	// Heightmap
	Heightmap	heightmap;
	glm::ivec2	lastRegionIndex;
	uint32_t	transferThreshold;
	uint32_t	regionCount;
	uint32_t	regionSize;

	// Vertex transfer
	std::queue<uint32_t> workIds;
	Buffer* compVertInactiveBuffer;
	VkFence transferFence;

	std::vector<Heightmap::Vertex> vertices;

	Texture depthTexture;
	RenderPass renderPass;
};