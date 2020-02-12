#include "jaspch.h"
#include "Image.h"
#include "Instance.h"

Image::Image()
{
}

Image::~Image()
{
}

void Image::init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	//If you want to be able to directly access texels in the memory of the image, then you must use VK_IMAGE_TILING_LINEAR
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (!queueFamilyIndices.empty())
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	else
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	imageInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();


	ERROR_CHECK(vkCreateImage(Instance::get().getDevice(), &imageInfo, nullptr, &this->image), "Failed to create image!");
}

void Image::bindImageMemory(VkDeviceMemory memory, VkDeviceSize offset)
{
	vkBindImageMemory(Instance::get().getDevice(), this->image, memory, offset);
}

VkImage Image::getImage()
{
	return this->image;
}

void Image::cleanup()
{
	vkDestroyImage(Instance::get().getDevice(), this->image, nullptr);
}
