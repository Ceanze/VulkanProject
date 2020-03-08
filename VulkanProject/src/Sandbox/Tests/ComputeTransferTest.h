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

#include <thread>
#include <mutex>

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
	void verticesToDevice(Buffer* buffer, const std::vector<glm::vec4>& verticies);

private:
	Camera* camera;
	CommandBuffer* cmdBuffs[3];
	CommandPool graphicsCommandPool;
	CommandPool transferCommandPool;

	RenderPass renderPass;

	DescriptorManager descManager;

	// For loading model
	Model defaultModel;
	Model transferModel;
	Buffer stagingBuffer;
	Memory stagingMemory;

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

	Buffer compVertBuffer;
	Buffer compVertBuffer2;
	Memory compVertMemory;

	Buffer indirectDrawBuffer;
	Memory indirectDrawMemory;
	uint32_t regionCount;
	Buffer compStagingBuffer;
	Memory compStagingMemory;
	Buffer planesUBO;
	Buffer worldDataUBO;
	Memory compUniformMemory;
	CommandBuffer* compCommandBuffers;

	Heightmap heightmap;
	glm::ivec2 lastRegionIndex;
	int transferThreshold;

	// Temp compute
	Buffer indexBufferTemp;
	Memory indexMemoryTemp;
	std::vector<glm::vec4> verticies;
};