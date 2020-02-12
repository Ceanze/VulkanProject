#pragma once

#include "jaspch.h"
#include <vulkan/vulkan.h>

class RenderPass;

class Framebuffer
{
public:
	Framebuffer();
	~Framebuffer();

	void init(size_t numFrameBuffers, RenderPass* renderpass, const std::vector<VkImageView>& attachments, VkExtent2D extent);

	VkFramebuffer getFramebuffer();

	void cleanup();

private:
	VkFramebuffer framebuffer;
};
