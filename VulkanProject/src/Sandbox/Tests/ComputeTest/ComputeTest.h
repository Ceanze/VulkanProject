#pragma once

#include "Core/Window.h"
#include "Core/Camera.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Pipeline/Shader.h"
#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Buffers/Framebuffer.h"
#include "Vulkan/Frame.h"

// Sandbox
#include "Sandbox/VKSandboxBase.h"

// TEMP
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Pipeline/PushConstants.h"

class ComputeTest : public VKSandboxBase
{
public:
	ComputeTest();
	~ComputeTest();

	void init();
	void loop(float dt);
	void cleanup();

	void setupDescriptors();
private:
	enum class Shaders {
		COMPUTE,
		PARTICLE,
		SIZE
	};

	enum class PipelineObject {
		COMPUTE,
		PARTICLE,
		SIZE
	};

	typedef PipelineObject PO;

	struct Particle {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 velocity;
	};
	std::vector<Particle> particles;

	void updateBufferDescs();
	void generateParticleData();
	void initComputePipeline();
	void initGraphicsPipeline();

	RenderPass renderPass;
	CommandPool commandPool;

	// Compute 
	DescriptorLayout computeDescLayout;
	Buffer particleBuffer;
	Memory particleMemory;

	Camera* camera;
	Buffer camBuffer;
	Memory memory;

	DescriptorLayout descLayout;
	DescriptorManager descManager;
	std::vector<PushConstants> pushConstants;

	CommandBuffer* cmdBuffs[3];
};