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
#include "Models/GLTFLoader.h"

class NoThreadingTest : public VKSandboxBase
{
public:
	void init() override;
	void loop(float dt) override;
	void cleanup() override;

private:
	struct PushConstantData
	{
		glm::mat4 mw;
		glm::vec4 tint;
	};

	struct ObjectData
	{
		glm::mat4 model;
		glm::mat4 world;
		glm::vec4 tint;
	};

	void prepareBuffers();
	void recordThread(uint32_t frameIndex);
	void updateBuffers(uint32_t frameIndex, float dt);

	void setupPre();
	void setupPost();

private:
	Camera* camera;
	CommandBuffer* cmdBuffs[3];
	CommandPool graphicsCommandPool;
	CommandPool transferCommandPool;

	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	Model model;
	GLTFLoader::StagingBuffers stagingBuffers;
	//Buffer stagingBuffer;
	//Memory stagingMemory;

	std::vector<CommandBuffer*> primaryBuffers;
	Texture depthTexture;
	Memory imageMemory; // Holds depth texture

	Buffer cameraBuffer;
	Memory memory;

	std::vector<ObjectData> objects;
};