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

// TEMP
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Pipeline/PushConstants.h"

class ComputeTest
{
public:
	ComputeTest();
	~ComputeTest();

	void init();
	void run();
	void shutdown();

	void setupPreTEMP();
	void setupPostTEMP();
private:
	void initComputePipeline();
	void initGraphicsPipeline();
	Window window;
	SwapChain swapChain;
	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<Framebuffer> framebuffers;
	Frame frame;

	Camera* camera;

	bool running = true;

	// TEMP
	CommandPool transferCommandPool;
	Buffer buffer;
	Buffer camBuffer;
	Memory memory;

	DescriptorLayout descLayout;
	DescriptorManager descManager;
	std::vector<PushConstants> pushConstants;
};