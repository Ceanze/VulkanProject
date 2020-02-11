#pragma once

#include <vulkan/vulkan.h>
#include "PipelineInfo.h"

class RenderPass;
class Shader;

class Pipeline
{
public:
	enum class Type {
		GRAPHICS = VK_PIPELINE_BIND_POINT_GRAPHICS,
		COMPUTE = VK_PIPELINE_BIND_POINT_COMPUTE
	};
public:
	Pipeline();
	~Pipeline();

	// Creates the pipeline with default values or the values that has been set before this call
	void init(Type type, Shader* shader);
	void cleanup();

	void setWireframe(bool enable);
	void setGraphicsPipelineInfo(VkExtent2D extent, RenderPass* renderPass);
	void setPipelineInfo(PipelineInfoFlag flags, PipelineInfo info);

private:
	void createGraphicsPipeline();
	void createComputePipeline();

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkPolygonMode polyMode;
	RenderPass* renderPass;
	Shader* shader;
	VkExtent2D extent;
	Type type;

	// Pipeline create info
	PipelineInfo graphicsPipelineInfo;
	PipelineInfoFlag graphicsPipelineInfoFlags;
};