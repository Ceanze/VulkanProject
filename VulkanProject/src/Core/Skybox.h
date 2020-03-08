#pragma once
#include "jaspch.h"

#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Vulkan/Sampler.h"
#include "Models/Model/Model.h"
#include "Vulkan/Pipeline/Shader.h"

class CommandBuffer;
class SwapChain;

class Skybox
{
public:
	struct CubemapUboData
	{
		glm::mat4 proj;
		glm::mat4 view;
	};
public:
	Skybox();
	~Skybox();

	void init(const std::string& texturePath, SwapChain* swapChain, CommandPool* pool);

	void draw(CommandBuffer* cmdBuff);

	void cleanup();

private:
	RenderPass renderPass;
	Pipeline pipeline;

	Shader shader;

	Buffer cubeBuffer;
	Memory cubeMemory;

	// ----------- Cubemap -----------
	Memory cubemapMemory;
	Texture cubemapTexture;
	Buffer cubemapStagingBuffer;
	Memory cubemapStagingMemory;
	Sampler cubemapSampler;
	DescriptorManager cubemapDescManager;
	Buffer cubemapUniformBuffer;

private:
	// Cube verticies
	glm::vec4 cube[36] = {
		{-1.0f,-1.0f,-1.0f, 1.0f},
		{-1.0f,-1.0f, 1.0f,	1.0f},
		{-1.0f, 1.0f, 1.0f, 1.0f},
		{ 1.0f, 1.0f,-1.0f, 1.0f},
		{-1.0f,-1.0f,-1.0f,	1.0f},
		{-1.0f, 1.0f,-1.0f, 1.0f},
		{ 1.0f,-1.0f, 1.0f,	1.0f},
		{-1.0f,-1.0f,-1.0f,	1.0f},
		{ 1.0f,-1.0f,-1.0f,	1.0f},
		{ 1.0f, 1.0f,-1.0f,	1.0f},
		{ 1.0f,-1.0f,-1.0f,	1.0f},
		{-1.0f,-1.0f,-1.0f,	1.0f},
		{-1.0f,-1.0f,-1.0f,	1.0f},
		{-1.0f, 1.0f, 1.0f,	1.0f},
		{-1.0f, 1.0f,-1.0f,	1.0f},
		{ 1.0f,-1.0f, 1.0f,	1.0f},
		{-1.0f,-1.0f, 1.0f,	1.0f},
		{-1.0f,-1.0f,-1.0f,	1.0f},
		{-1.0f, 1.0f, 1.0f,	1.0f},
		{-1.0f,-1.0f, 1.0f,	1.0f},
		{ 1.0f,-1.0f, 1.0f,	1.0f},
		{ 1.0f, 1.0f, 1.0f,	1.0f},
		{ 1.0f,-1.0f,-1.0f,	1.0f},
		{ 1.0f, 1.0f,-1.0f,	1.0f},
		{ 1.0f,-1.0f,-1.0f, 1.0f},
		{ 1.0f, 1.0f, 1.0f, 1.0f},
		{ 1.0f,-1.0f, 1.0f, 1.0f},
		{ 1.0f, 1.0f, 1.0f, 1.0f},
		{ 1.0f, 1.0f,-1.0f, 1.0f},
		{-1.0f, 1.0f,-1.0f,	1.0f},
		{ 1.0f, 1.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f,-1.0f,	1.0f},
		{-1.0f, 1.0f, 1.0f,	1.0f},
		{ 1.0f, 1.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f,	1.0f},
		{ 1.0f,-1.0f, 1.0f, 1.0f}
	};
};

