#include "jaspch.h"
#include "ImageView.h"
#include "Vulkan/Instance.h"



ImageView::ImageView()
{
}

ImageView::~ImageView()
{
}

void ImageView::init(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectMask, uint32_t layerCount)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;

	createInfo.viewType = type;
	createInfo.format = format;

	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = layerCount;

	ERROR_CHECK(vkCreateImageView(Instance::get().getDevice(), &createInfo, nullptr, &this->imageView), "Failed to create image view!");
}

void ImageView::cleanup()
{
	vkDestroyImageView(Instance::get().getDevice(), this->imageView, nullptr);
}
