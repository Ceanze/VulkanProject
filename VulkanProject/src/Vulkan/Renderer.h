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
#include "Core/Camera.h"

// TEMP
#include "Buffers/Buffer.h"
#include "Buffers/Memory.h"
#include "Pipeline/DescriptorManager.h"
#include "Texture.h"
#include "Sampler.h"
#include "Buffers/Image.h"
#include "Buffers/ImageView.h"

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

	Camera* camera;

	bool running = true;

	// TEMP
	Pipeline offscreenPipeline;
	Shader offscreenShader;
	RenderPass offScreenRenderPass;
	std::vector<Framebuffer> offScreenFramebuffers;
	Sampler offScreenSampler;
	std::vector<Texture> offScreenTextures;
	DescriptorLayout offScreenDescLayout;
	DescriptorManager offScreenDescManager;
	Buffer offScreenBuffer;
	Buffer camBuffer;
	Memory memoryTexture;
	std::vector<Sampler> samplers;

	Buffer buffer;
	Memory memory;
	Memory memoryTexture2;
	Buffer stagingBuffer;
	Texture texture;
	Sampler sampler;
	DescriptorLayout descLayout;
	DescriptorManager descManager;
};