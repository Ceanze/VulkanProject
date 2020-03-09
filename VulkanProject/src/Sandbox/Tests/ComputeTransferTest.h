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
	const int MESH_PIPELINE = 0;
	const int COMP_PIPELINE = 1;

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

	void buildComputeCommandBuffer();
	void record();
	
	void generateHeightmap();
	void verticesToDevice(Buffer* buffer, const std::vector<Heightmap::Vertex>& verticies);

	void transferVertexData();

private:
	Camera* camera;
	CommandBuffer* compCommandBuffer;
	CommandBuffer* cmdBuffs[3];
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

	std::thread* thread;
	std::mutex mutex;
	bool isTransferDone;

	// Compute
	DescriptorManager descManagerComp;
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

	Skybox skybox;
};