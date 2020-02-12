#pragma once

#include "Core/Window.h"
#include "SwapChain.h"
#include "Pipeline/Shader.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderPass.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Buffers/Framebuffer.h"

class Renderer
{
public:
	Renderer();
	~Renderer();

	void init();
	void run();
	void shutdown();

private:
	Window window;
	SwapChain swapChain;
	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<Framebuffer> framebuffers;

	bool running = true;
};