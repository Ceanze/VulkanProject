#include "jaspch.h"
#include "Texture.h"
#include "Vulkan/Instance.h"

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices)
{
	this->image.init(width, height, format, usage, queueFamilyIndices);
	//this->imageView.init(getVkImage(), VK_IMAGE_VIEW_TYPE_2D, format);
}

void Texture::cleanup()
{
	this->imageView.cleanup();
	this->image.cleanup();
}

VkMemoryRequirements Texture::getMemReq() const
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Instance::get().getDevice(), this->image.getImage() , &memRequirements);

	return memRequirements;
}

VkImage Texture::getVkImage() const
{
	return this->image.getImage();
}

Image& Texture::getImage()
{
	return this->image;
}

VkImageView Texture::getVkImageView() const
{
	return this->imageView.getImageView();
}

ImageView& Texture::getImageView()
{
	return this->imageView;
}
