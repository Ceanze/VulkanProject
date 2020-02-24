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
#include "Threading/ThreadManager.h"

class ThreadingTest : public VKSandboxBase
{
public:
	void init() override;
	void loop(float dt) override;
	void cleanup() override;

private:
	struct PushConstantData
	{
		glm::mat4 mw;
		glm::mat4 vp;
		glm::vec4 tint;
	};

	struct ObjectData
	{
		glm::mat4 model;
		glm::mat4 world;
		glm::vec4 tint;
	};

	// Data per thread.
	struct ThreadData
	{
		CommandPool cmdPool;
		std::vector<CommandBuffer*> cmdBuffs;
		std::vector<ObjectData> objects;
		std::vector<PushConstants> pushConstants;
	};

	void prepareBuffers();
	void recordThread(uint32_t threadId, uint32_t frameIndex, VkCommandBufferInheritanceInfo inheritanceInfo);
	void updateBuffers(uint32_t frameIndex, float dt);

	void setupPre();
	void setupPost();

private:
	Camera* camera;
	CommandBuffer* cmdBuffs[3];
	CommandPool graphicsCommandPool;

	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	Model model;

	std::vector<CommandBuffer*> primaryBuffers;
	Texture depthTexture;
	Memory imageMemory; // Holds depth texture

	uint32_t numThreads;
	ThreadManager threadManager;
	std::vector<PushConstants> pushConstants;
	std::vector<ThreadData> threadData;
};