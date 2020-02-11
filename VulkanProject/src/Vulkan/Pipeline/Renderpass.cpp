#include "jaspch.h"
#include "Renderpass.h"

#include "../VulkanCommon.h"
#include "../Instance.h"

RenderPass::RenderPass() : renderPass(VK_NULL_HANDLE)
{
}

RenderPass::~RenderPass()
{
}

void RenderPass::addDepthAttachment(VkAttachmentDescription desc)
{
	this->attachments.push_back(desc);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = (uint32_t)(this->attachments.size()-1);
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	this->attachmentRefs.push_back(colorAttachmentRef);
}

void RenderPass::addColorAttachment(VkAttachmentDescription desc)
{
	this->attachments.push_back(desc);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = (uint32_t)(this->attachments.size() - 1);
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	this->attachmentRefs.push_back(colorAttachmentRef);
}

void RenderPass::addSubpass()
{
	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	//subpassDesc.pColorAttachments = &colorAttachmentRef;
	//subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	subpasses.push_back(subpassDesc);
}

void RenderPass::addSubpassDependency(VkSubpassDependency dependency)
{
	this->subpassDependencies.push_back(dependency);
}

void RenderPass::init(VkFormat swapChainImageFormat)
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat(Instance::get().getPhysicalDevice());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(this->attachments.size());
	renderPassInfo.pAttachments = this->attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = subpassDependencies.size();
	renderPassInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(Instance::get().getDevice(), &renderPassInfo, nullptr, &this->renderPass);
	ERROR_CHECK(result == VK_SUCCESS, "Failed to create render pass!");
}

void RenderPass::cleanup()
{
	vkDestroyRenderPass(Instance::get().getDevice(), this->renderPass, nullptr);
}
