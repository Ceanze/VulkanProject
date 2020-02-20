#include "jaspch.h"
#include "Framebuffer.h"

#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/Instance.h"

Framebuffer::Framebuffer()
{
}

Framebuffer::~Framebuffer()
{
}

void Framebuffer::init(size_t numFrameBuffers, RenderPass* renderpass, const std::vector<VkImageView>& attachments, VkExtent2D extent)
{
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderpass->getRenderPass();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;

	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();

	ERROR_CHECK(vkCreateFramebuffer(Instance::get().getDevice(), &framebufferInfo, nullptr, &this->framebuffer), "Failed to create framebuffer");
}

VkFramebuffer Framebuffer::getFramebuffer()
{
	return this->framebuffer;
}

void Framebuffer::cleanup()
{
	vkDestroyFramebuffer(Instance::get().getDevice(), this->framebuffer, nullptr);
}
