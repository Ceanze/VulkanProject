#include "jaspch.h"
#include "RenderPass.h"

#include "../VulkanCommon.h"
#include "../Instance.h"

RenderPass::RenderPass() : renderPass(VK_NULL_HANDLE)
{
}

RenderPass::~RenderPass()
{
}

void RenderPass::addColorAttachment(VkAttachmentDescription desc)
{
	this->attachments.push_back(desc);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = (uint32_t)(this->attachments.size() - 1);
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	this->attachmentRefs[colorAttachmentRef.attachment] = colorAttachmentRef;
}

void RenderPass::addDepthAttachment(VkAttachmentDescription desc)
{
	this->attachments.push_back(desc);

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = (uint32_t)(this->attachments.size()-1);
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	this->attachmentRefs[depthAttachmentRef.attachment] = depthAttachmentRef;
}

void RenderPass::addDefaultColorAttachment(VkFormat swapChainImageFormat)
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
	addColorAttachment(colorAttachment);
}

void RenderPass::addDefaultDepthAttachment()
{
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat(Instance::get().getPhysicalDevice());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	addDepthAttachment(depthAttachment);
}

void RenderPass::addSubpass(SubpassInfo info)
{
	SubpassInternal tmp;
	this->subpasseInternals.push_back(tmp);
	SubpassInternal& subpassInternal = this->subpasseInternals.back();

	// Fill vectors with attachment references.
	for(uint32_t i : info.inputAttachmentIndices)
		subpassInternal.inputAttachments.push_back(getRef(i));
	for (uint32_t i : info.colorAttachmentIndices)
		subpassInternal.colorAttachments.push_back(getRef(i));
	for (uint32_t i : info.preserveAttachments)
		subpassInternal.preserveAttachments.push_back(i);

	subpassInternal.hasDepthStencilAttachment = info.depthStencilAttachmentIndex == -1 ? false : true;
	if(info.depthStencilAttachmentIndex != -1)
		subpassInternal.depthStencilAttachment = getRef(info.depthStencilAttachmentIndex);
	subpassInternal.hasResolveAttachment = info.resolveAttachmentIndex == -1 ? false : true;
	if (info.resolveAttachmentIndex != -1)
		subpassInternal.resolveAttachment = getRef(info.resolveAttachmentIndex);
}

void RenderPass::addSubpassDependency(VkSubpassDependency dependency)
{
	this->subpassDependencies.push_back(dependency);
}

void RenderPass::addDefaultSubpassDependency()
{
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	addSubpassDependency(dependency);
}

void RenderPass::init()
{
	// Check if data is set.
	JAS_ASSERT(this->attachments.empty() == false, "RenderPass need at least one attachment!"); // This is not the case in the documentation, but I think it should.
	JAS_ASSERT(this->subpasseInternals.empty() == false, "RenderPass need at least one subpass!");
	if (this->subpassDependencies.empty())
		JAS_WARN("RenderPass has no subpass dependency.");

	// Construct the subpasses.
	std::vector<VkSubpassDescription> subpasses;
	for (auto& sub : this->subpasseInternals)
	{
		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.inputAttachmentCount = static_cast<uint32_t>(sub.inputAttachments.size());
		subpassDesc.pInputAttachments = sub.inputAttachments.data();
		subpassDesc.colorAttachmentCount = static_cast<uint32_t>(sub.colorAttachments.size());
		subpassDesc.pColorAttachments = sub.colorAttachments.data();
		subpassDesc.preserveAttachmentCount = static_cast<uint32_t>(sub.preserveAttachments.size());
		subpassDesc.pPreserveAttachments = sub.preserveAttachments.data();
		if(sub.hasDepthStencilAttachment)
			subpassDesc.pDepthStencilAttachment = &sub.depthStencilAttachment;
		if(sub.hasResolveAttachment)
			subpassDesc.pResolveAttachments = &sub.resolveAttachment;

		subpasses.push_back(subpassDesc);
	}

	// Create the render pass.
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(this->attachments.size());
	renderPassInfo.pAttachments = this->attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(this->subpassDependencies.size());
	renderPassInfo.pDependencies = this->subpassDependencies.data();

	VkResult result = vkCreateRenderPass(Instance::get().getDevice(), &renderPassInfo, nullptr, &this->renderPass);
	JAS_ASSERT(result == VK_SUCCESS, "Failed to create render pass!");
}

void RenderPass::cleanup()
{
	vkDestroyRenderPass(Instance::get().getDevice(), this->renderPass, nullptr);
}

VkRenderPass RenderPass::getRenderPass()
{
	return this->renderPass;
}

VkAttachmentReference RenderPass::getRef(uint32_t index)
{
	auto it = this->attachmentRefs.find(index);
	JAS_ASSERT(it != this->attachmentRefs.end(), "Try to reference an attachment which does not exist!");
	return it->second;
}
