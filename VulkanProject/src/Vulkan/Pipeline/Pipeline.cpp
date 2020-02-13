#include "jaspch.h"
#include "Pipeline.h"
#include "../Instance.h"
#include "Shader.h"
#include "RenderPass.h"
#include "DescriptorLayout.h"

// TODO: Add support for addition of descriptors and push constants for both graphics and compute pipeline.

Pipeline::Pipeline() :
	pipeline(VK_NULL_HANDLE), polyMode(VK_POLYGON_MODE_FILL),
	graphicsPipelineInfoFlags(PipelineInfoFlag::NONE)
{
}

Pipeline::~Pipeline()
{
}

void Pipeline::init(Type type, Shader* shader)
{
	this->type = type;
	this->shader = shader;

	if (type == Type::GRAPHICS)
		createGraphicsPipeline();
	else if (type == Type::COMPUTE)
		createComputePipeline();
}

void Pipeline::cleanup()
{
	VkDevice device = Instance::get().getDevice();
	vkDestroyPipeline(device, this->pipeline, nullptr);
	vkDestroyPipelineLayout(device, this->pipelineLayout, nullptr);
}

void Pipeline::setDescriptorLayouts(const std::vector<DescriptorLayout>& descriptorLayouts)
{
	for (auto& layout : descriptorLayouts)
		this->layouts.push_back(layout.getLayout());
}

void Pipeline::setWireframe(bool enable)
{
	if (enable)
		this->polyMode = VK_POLYGON_MODE_LINE;
	else
		this->polyMode = VK_POLYGON_MODE_FILL;
}

void Pipeline::setGraphicsPipelineInfo(VkExtent2D extent, RenderPass* renderPass)
{
	this->renderPass = renderPass;
	this->extent = extent;
	this->graphicsPipelineInitilized = true;
}

void Pipeline::setPipelineInfo(PipelineInfoFlag flags, PipelineInfo info)
{
	this->graphicsPipelineInfoFlags = this->graphicsPipelineInfoFlags | flags;

	if (flags & PipelineInfoFlag::VERTEX_INPUT)
		this->graphicsPipelineInfo.vertexInputInfo = info.vertexInputInfo;
	if (flags & PipelineInfoFlag::RASTERIZATION)
		this->graphicsPipelineInfo.rasterizer = info.rasterizer;
	if (flags & PipelineInfoFlag::MULTISAMPLE)
		this->graphicsPipelineInfo.multisampling = info.multisampling;
	if (flags & PipelineInfoFlag::DEAPTH_STENCIL)
		this->graphicsPipelineInfo.depthStencil = info.depthStencil;
	if (flags & PipelineInfoFlag::COLOR_BLEND_ATTACHMENT)
		this->graphicsPipelineInfo.colorBlendAttachment = info.colorBlendAttachment;
	if (flags & PipelineInfoFlag::COLOR_BLEND)
		this->graphicsPipelineInfo.colorBlending = info.colorBlending;
}

void Pipeline::createGraphicsPipeline()
{
	if (!this->graphicsPipelineInitilized) {
		JAS_ERROR("Couldn't create graphics pipeline: Renderpass and extent not set! To set this use the setGraphicsPipelineInfo function call.");
		return;
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)this->extent.width;
	viewport.height = (float)this->extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = this->extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Used to set up uniform buffers and push constants
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(this->layouts.size());
	pipelineLayoutInfo.pSetLayouts = this->layouts.empty() ? nullptr : this->layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	// Vertex attribute info for vertex shaders
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	if (this->graphicsPipelineInfoFlags & PipelineInfoFlag::VERTEX_INPUT)
		vertexInputInfo = this->graphicsPipelineInfo.vertexInputInfo;
	else {
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	}

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	if (this->graphicsPipelineInfoFlags & PipelineInfoFlag::RASTERIZATION)
		rasterizer = this->graphicsPipelineInfo.rasterizer;
	else {
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;			 // If true fragments that are beyond the near and far planes are clamped to them as opposed to discarding them
		rasterizer.rasterizerDiscardEnable = VK_FALSE;	 // If true disables any output to the framebuffer
		rasterizer.polygonMode = this->polyMode;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
	}

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	if (this->graphicsPipelineInfoFlags & PipelineInfoFlag::MULTISAMPLE)
		multisampling = this->graphicsPipelineInfo.multisampling;
	else {
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional
	}

	// Depth and stencil information
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	if (this->graphicsPipelineInfoFlags & PipelineInfoFlag::DEAPTH_STENCIL)
		depthStencil = this->graphicsPipelineInfo.depthStencil;
	else {
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional
	}

	// Color blending options
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	if (this->graphicsPipelineInfoFlags & PipelineInfoFlag::COLOR_BLEND_ATTACHMENT)
		colorBlendAttachment = this->graphicsPipelineInfo.colorBlendAttachment;
	else {
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		// These parameters will be ignored, but they are used for alpha blending depending on opacity
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	if (this->graphicsPipelineInfoFlags & PipelineInfoFlag::COLOR_BLEND)
		colorBlending = this->graphicsPipelineInfo.colorBlending;
	else {
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional
	}

	ERROR_CHECK(vkCreatePipelineLayout(Instance::get().getDevice(), &pipelineLayoutInfo, nullptr, &this->pipelineLayout), "Failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	std::vector<VkPipelineShaderStageCreateInfo> infos = this->shader->getShaderCreateInfos();
	pipelineInfo.stageCount = infos.size();
	pipelineInfo.pStages = infos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = this->pipelineLayout;
	pipelineInfo.renderPass = this->renderPass->getRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.flags = 0;
	pipelineInfo.basePipelineIndex = -1; // Optional

	ERROR_CHECK(vkCreateGraphicsPipelines(Instance::get().getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline), "Failed to create graphics pipeline!");
}

void Pipeline::createComputePipeline()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // descriptor.getSetLayouts().size();
	pipelineLayoutInfo.pSetLayouts = nullptr; // descriptor.getSetLayouts().data(); // Uniform buffer objects, images, etc.
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	ERROR_CHECK(vkCreatePipelineLayout(Instance::get().getDevice(), &pipelineLayoutInfo, nullptr, &this->pipelineLayout), "Failed to create pipeline layout!");

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stage = this->shader->getShaderCreateInfo(Shader::Type::COMPUTE);
	pipelineInfo.layout = this->pipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	ERROR_CHECK(vkCreateComputePipelines(Instance::get().getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline), "Failed to create graphics pipeline!");
}