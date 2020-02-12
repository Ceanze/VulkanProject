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

class Renderer
{
public:
	Renderer();
	~Renderer();

	void init();
	void run();
	void shutdown();

private:
	uint32_t beginFrame();
	void endFrame(uint32_t imageIndex);

	Window window;
	SwapChain swapChain;
	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<Framebuffer> framebuffers;
	Frame frame;

	bool running = true;
};