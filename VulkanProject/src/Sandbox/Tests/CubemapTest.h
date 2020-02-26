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
	struct UboData
	{
		glm::mat4 world;
		glm::mat4 vp;
	};

	void setupPre();
	void setupPost();
	void setupCubemap();

private:
	Camera* camera;
	CommandBuffer* cmdBuffs[3];
	CommandPool graphicsCommandPool;

	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	// For loading model
	Model model;

	Buffer bufferUniform;
	Texture depthTexture;
	Memory memory; // Holds uniform data
	Memory imageMemory; // Holds depth texture

	// ----------- Cubemap -----------
	Memory cubemapMemory;
	Texture cubemapTexture;
	Memory cubemapStagingMemory;
	Buffer cubemapStagineBuffer;
};