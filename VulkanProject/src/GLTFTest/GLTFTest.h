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

// ------------ TEMP ------------
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "glTF/tiny_gltf.h"

#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"

class GLTFTest
{
public:
	GLTFTest();
	~GLTFTest();

	void init();
	void run();
	void cleanup();

private:
	void setupPreTEMP();
	void setupPostTEMP();

	void loadModel(const std::string& filePath);

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

	// For loading of model

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	CommandBuffer* cmdBuffs[3];
	Buffer bufferPos;
	Buffer bufferUniform;
	Memory memory; // Holds vertex and index data.
};