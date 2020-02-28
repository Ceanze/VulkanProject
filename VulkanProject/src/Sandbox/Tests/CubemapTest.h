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

class CubemapTest : public VKSandboxBase
{
public:
	void init() override;
	void loop(float dt) override;
	void cleanup() override;

private:
	static const uint32_t MAIN_PIPELINE = 0;
	static const uint32_t MAIN_SHADER = 0;
	static const uint32_t CUBEMAP_PIPELINE = 1;
	static const uint32_t CUBEMAP_SAHDER = 1;

	struct UboData
	{
		glm::mat4 world;
		glm::mat4 vp;
		alignas(16) glm::vec3 camPos;
	};

	struct CubemapUboData
	{
		glm::mat4 proj;
		glm::mat4 view;
	};

	void setupPre();
	void setupPost();
	void setupCubemap();

private:
	Camera* camera;
	CommandBuffer* cmdBuffs[3];
	CommandPool graphicsCommandPool;

	RenderPass renderPass;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	// For loading model
	Model model;
	Model cube;

	Buffer bufferUniform;
	Texture depthTexture;
	Memory memory; // Holds uniform data
	Memory imageMemory; // Holds depth texture

	// ----------- Cubemap -----------
	Memory cubemapMemory;
	Texture cubemapTexture;
	Memory cubemapStagingMemory;
	Buffer cubemapStagineBuffer;
	Sampler cubemapSampler;
	DescriptorManager cubemapDescManager;
	Buffer cubemapUniformBuffer;
};