#pragma once

#include "Core/Camera.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Pipeline/DescriptorManager.h"
#include "Vulkan/Texture.h"
#include "Core/Heightmap/Heightmap.h"

// Sandbox
#include "Sandbox/VKSandboxBase.h"


class HeightmapTest : public VKSandboxBase
{
public:
	void init();
	void loop(float dt);
	void cleanup();

	void setupDescriptors();
private:
	enum class Shaders {
		HEIGTHMAP,
		SIZE
	};

	enum class PipelineObject {
		HEIGTHMAP,
		SIZE
	};

	typedef PipelineObject PO;

	void updateBufferDescs();
	void generateHeightmapData();
	void initGraphicsPipeline();

	RenderPass renderPass;
	CommandPool commandPool;

	Heightmap heightmap;
	Buffer indexBuffer;
	Buffer heightBuffer;
	Camera* camera;
	Buffer camBuffer;
	Memory memory;

	Texture depthTexture;
	Memory imageMemory;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	CommandBuffer* cmdBuffs[3];
};