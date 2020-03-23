#pragma once
#include "jaspch.h"

#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Vulkan/Sampler.h"
#include "Models/Model/Model.h"
#include "Vulkan/Pipeline/Shader.h"

class CommandBuffer;
class SwapChain;
class Camera;
class RenderPass;

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

	void init(float scale, const std::string& texturePath, SwapChain* swapChain, CommandPool* pool, RenderPass* renderPass);

	void update(Camera* camera);
	void draw(CommandBuffer* cmdBuff, uint32_t frameIndex);

	Buffer* getBuffer();

	void cleanup();

	Pipeline& getPipeline();

private:
	Pipeline pipeline;
	Shader shader;

	Buffer cubeBuffer;
	Memory cubeMemory;

	// ----------- Cubemap -----------
	Memory cubemapMemory;
	Texture cubemapTexture;
	Sampler cubemapSampler;
	DescriptorManager cubemapDescManager;
	Buffer cubemapUniformBuffer;
	Memory uniformMemory;

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

