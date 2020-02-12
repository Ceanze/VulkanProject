#pragma once

#include "jaspch.h"
#include <vulkan/vulkan.h>

class Framebuffer
{
public:
	Framebuffer();
	~Framebuffer();

	void init(size_t numFrameBuffers, VkRenderPass renderpass, const std::vector<VkImageView>& attachments, uint32_t width, uint32_t height, uint32_t layers);

	std::vector<VkFramebuffer> getFramebuffers();

	void cleanup();

private:
	std::vector<VkFramebuffer> framebuffers;
};