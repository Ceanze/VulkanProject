#include "jaspch.h"
#include "Framebuffer.h"

#include "Instance.h"

Framebuffer::Framebuffer()
{
}

Framebuffer::~Framebuffer()
{
}

void Framebuffer::init(size_t numFrameBuffers, VkRenderPass renderpass, const std::vector<VkImageView>& attachments, uint32_t width, uint32_t height, uint32_t layers)
{
	this->framebuffers.resize(numFrameBuffers);

	for (size_t i = 0; i < numFrameBuffers; i++)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderpass;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = layers;

		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();

		ERROR_CHECK(vkCreateFramebuffer(Instance::get().getDevice(), &framebufferInfo, nullptr, &this->framebuffers[i]), "Failed to create framebuffer");
	}
}

std::vector<VkFramebuffer> Framebuffer::getFramebuffers()
{
	return this->framebuffers;
}

void Framebuffer::cleanup()
{
	for (auto framebuffer : this->framebuffers) {
		vkDestroyFramebuffer(Instance::get().getDevice(), framebuffer, nullptr);
	}
}
