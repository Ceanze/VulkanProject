#pragma once

#include "Core/Window.h"
#include "SwapChain.h"
#include "Pipeline/Shader.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderPass.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Buffers/Framebuffer.h"
#include "Frame.h"

// TEMP
#include "Buffers/Buffer.h"
#include "Buffers/Memory.h"
#include "Pipeline/DescriptorManager.h"
#include "Texure.h"
#include "Sampler.h"

class Renderer
{
public:
	Renderer();
	~Renderer();

	void init();
	void run();
	void shutdown();

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

	// TEMP
	Buffer buffer;
	Memory memory;
	Buffer stagingBuffer;
	Texture texture;
	Sampler sampler;
	DescriptorLayout descLayout;
	DescriptorManager descManager;
};