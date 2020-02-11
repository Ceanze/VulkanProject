#pragma once

#include "jaspch.h"

class RenderPass
{
public:
	struct Subpass
	{
		VkSubpassDescription desc;
	};

public:
	RenderPass();
	~RenderPass();

	void addDepthAttachment(VkAttachmentDescription desc);
	void addColorAttachment(VkAttachmentDescription desc);
	void addSubpass();
	void addSubpassDependency(VkSubpassDependency dependency);
	void init(VkFormat swapChainImageFormat);

	void cleanup();

private:
	VkRenderPass renderPass;
	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkSubpassDependency> subpassDependencies;
	std::vector<VkAttachmentReference> attachmentRefs;
	std::vector<VkSubpassDescription> subpasses;
};