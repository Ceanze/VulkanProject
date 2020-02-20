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
	struct Particle {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 velocity;
	};
	std::vector<Particle> particles;
	std::vector<glm::vec4> particleColors;

	void generateParticleData();
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

	// Compute 
	Shader computeShader;
	Pipeline computePipeline;
	CommandPool computeCommandPool;
	DescriptorLayout computeDescLayout;
	DescriptorManager computeDescManager;
	Buffer particleBuffer;
	Memory particleMemory;

	Camera* camera;
	bool running = true;

	// TEMP
	Buffer camBuffer;
	Memory memory;
	DescriptorLayout descLayout;
	DescriptorManager descManager;
	std::vector<PushConstants> pushConstants;
};