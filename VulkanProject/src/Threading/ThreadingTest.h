#pragma once

#include "Core/Window.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Pipeline/Shader.h"
#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Buffers/Framebuffer.h"
#include "Vulkan/Frame.h"

#include "Vulkan/Pipeline/DescriptorManager.h"

#include "Models/Model/Model.h"

#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Core/Camera.h"

#include "Threading/ThreadManager.h"
#include "Vulkan/Pipeline/PushConstants.h"

class ThreadingTest
{
public:
	ThreadingTest();
	~ThreadingTest();

	void init();
	void run();
	void shutdown();

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

	void setupPreTEMP();
	void setupPostTEMP();

private:
	Window window;
	SwapChain swapChain;
	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<Framebuffer> framebuffers;
	Frame frame;

	bool running = true;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	Camera* camera;

	// For loading model
	Model model;

	std::vector<CommandBuffer*> primaryBuffers;
	Texture depthTexture;
	Memory imageMemory; // Holds depth texture

	uint32_t numThreads;
	ThreadManager threadManager;
	std::vector<PushConstants> pushConstants;
	std::vector<ThreadData> threadData;
};