#pragma once

#include "Sandbox/VKSandboxBase.h"

#include "Core/Camera.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Texture.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Pipeline/PushConstants.h"
#include "Models/Model/Model.h"
#include "Core/Heightmap/Heightmap.h"
#include "Core/Skybox.h"

#include <queue>

class ComputeTransferTest : public VKSandboxBase
{
public:
	void init() override;
	void loop(float dt) override;
	void cleanup() override;

private:
	const int MESH_SHADER = 0;
	const int COMP_SHADER = 1;
	const int COMP_INDEX_SHADER = 2;
	const int MESH_PIPELINE = 0;
	const int COMP_PIPELINE = 1;
	const int COMP_INDEX_PIPELINE = 2;

	struct ComputeIndexConfig {
		uint32_t regionCount;  // Number of regions in width for proximity
		uint32_t regionSize;   // Number of vertices in width for one region.
		uint32_t proximitySize;
		uint32_t pad2;
	};

	struct UboData
	{
		//glm::mat4 world;
		glm::mat4 vp;
	};

	struct WorldData
	{
		uint32_t regWidth;           // Region width in number of vertices.
		uint32_t numIndicesPerReg;   // Number of indices per region.
		uint32_t loadedWidth;        // Loaded world width in verticies
		uint32_t regionCount;
	};

	void setupPre();
	void setupPost();
	void setupCompute();
	void setupComputeIndex();

	void buildComputeCommandBuffer(uint32_t id);
	void buildComputeIndexCommandBuffer();
	void record(uint32_t id);
	
	void generateHeightmap();
	void verticesToDevice(Buffer* buffer, const std::vector<Heightmap::Vertex>& verticies);

	void transferVertexData();

private:
	Camera* camera;
	uint32_t activeBuffers;
	CommandBuffer* compIndexCommandBuffer;
	std::array<CommandBuffer*, 2> compCommandBuffer;
	std::array<std::array<CommandBuffer*, 3>, 2> cmdBuffs;
	CommandPool graphicsCommandPool;
	CommandPool transferCommandPool;

	RenderPass renderPass;

	DescriptorManager descManager;

	// For loading model
	Model defaultModel;
	Model transferModel;

	Buffer bufferUniform;
	Texture depthTexture;
	Memory memory; // Holds uniform data
	Memory imageMemory; // Holds depth texture

	// Compute
	DescriptorManager descManagerComp;
	DescriptorManager descManagerCompIndex;
	CommandPool compCommandPool;

	Buffer vertStagingBuffer;
	Memory vertStagingMemory;

	std::queue<uint32_t> workIds;
	Buffer* compVertInactiveBuffer;
	Buffer compVertBuffer;
	Buffer compVertBuffer2;
	Memory compVertMemory;

	Buffer indirectDrawBuffer;
	Memory indirectDrawMemory;

	Buffer compStagingBuffer;
	Memory compStagingMemory;

	Buffer planesUBO;
	Buffer worldDataUBO;
	Memory compUniformMemory;

	// Heightmap stuff
	Heightmap heightmap;
	glm::ivec2 lastRegionIndex;
	int transferThreshold;
	uint32_t regionCount;

	// Temp compute
	Buffer indexBuffer;
	Memory indexMemory;
	std::vector<Heightmap::Vertex> verticies;

	// Compute Index
	Buffer indexBufferCompute;
	Buffer configBuffer;
	
	Skybox skybox;
};