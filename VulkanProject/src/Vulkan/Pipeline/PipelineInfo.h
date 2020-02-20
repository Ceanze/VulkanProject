#pragma once

enum class PipelineInfoFlag {
	NONE = 0,
	VERTEX_INPUT = 1,
	RASTERIZATION = 2,
	MULTISAMPLE = 4,
	DEAPTH_STENCIL = 8,
	COLOR_BLEND_ATTACHMENT = 16,
	COLOR_BLEND = 32,
	INPUT_ASSEMBLY = 64,
	VIEWPORT = 128,
	SCISSOR = 256
};

inline int operator&(PipelineInfoFlag a, PipelineInfoFlag b)
{
	return static_cast<int>(static_cast<int>(a) & static_cast<int>(b));
}

inline PipelineInfoFlag operator|(PipelineInfoFlag a, PipelineInfoFlag b)
{
	return static_cast<PipelineInfoFlag>(static_cast<int>(a) | static_cast<int>(b));
}

struct PipelineInfo 
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlending;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;

	PipelineInfo& PipelineInfo::operator =(const PipelineInfo& other)
	{
		this->vertexInputInfo = other.vertexInputInfo;
		this->rasterizer = other.rasterizer;
		this->multisampling = other.multisampling;
		this->depthStencil = other.depthStencil;
		this->colorBlendAttachment = other.colorBlendAttachment;
		this->colorBlending = other.colorBlending;
		this->inputAssembly = other.inputAssembly;
		this->viewport = other.viewport;
		this->scissor = other.scissor;
	}
};