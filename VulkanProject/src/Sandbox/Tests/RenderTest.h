#pragma once

#include "Sandbox/VKSandboxBase.h"

#include "Core/Camera.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Texture.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Pipeline/PushConstants.h"

class RenderTest : public VKSandboxBase
{
public:
	void init() override;
	void loop(float dt) override;
	void cleanup() override;

private:
	void setupPre();
	void setupPost();

private:
	Camera* camera;
	CommandBuffer* cmdBuffs[3];
	CommandPool graphicsCommandPool;
	CommandPool transferCommandPool;

	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;

	Buffer buffer;
	Buffer camBuffer;
	Memory memory;
	Memory memoryTexture;
	Buffer stagingBuffer;
	Texture texture;
	Sampler sampler;
	DescriptorLayout descLayout;
	DescriptorManager descManager;
	PushConstants pushConstants;
};