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

#include "Models/GLTFLoader.h"
#include "Models/Model/Model.h"

#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Core/Camera.h"

#include "Threading/ThreadManager.h"

class ThreadingTest
{
public:
	ThreadingTest();
	~ThreadingTest();

	void init();
	void run();
	void shutdown();

private:
	struct UboData
	{
		glm::mat4 world;
		glm::mat4 vp;
	};

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
	GLTFLoader loader;
	Model model;

	CommandBuffer* cmdBuffs[3];
	Buffer bufferUniform;
	Texture depthTexture;
	Memory memory; // Holds uniform data
	Memory imageMemory; // Holds depth texture

	ThreadManager threadManager;
};