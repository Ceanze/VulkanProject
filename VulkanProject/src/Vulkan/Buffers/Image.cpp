#include "jaspch.h"
#include "Image.h"
#include "Vulkan/Instance.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/CommandPool.h"
#include "Buffer.h"

Image::Image() : width(0), height(0), image(VK_NULL_HANDLE), layout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

Image::~Image()
{
}

void Image::init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices)
{
	this->width = width;
	this->height = height;

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

	if (queueFamilyIndices.size() > 1)
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	else
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	imageInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();


	ERROR_CHECK(vkCreateImage(Instance::get().getDevice(), &imageInfo, nullptr, &this->image), "Failed to create image!");
}

void Image::transistionLayout(TransistionDesc& desc)
{
	this->layout = desc.newLayout;

	CommandBuffer* buffer = desc.pool->beginSingleTimeCommand();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = desc.oldLayout;
	barrier.newLayout = desc.newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = this->image;

	if (desc.newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		// Check if format has a stencil component
		if (desc.format == VK_FORMAT_D32_SFLOAT_S8_UINT || desc.format == VK_FORMAT_D24_UNORM_S8_UINT) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (desc.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && desc.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (desc.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && desc.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (desc.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && desc.newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		JAS_ASSERT(false, "Unsupported layout transistion!");
	}

	vkCmdPipelineBarrier(buffer->getCommandBuffer(), sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	desc.pool->endSingleTimeCommand(buffer);
}

void Image::copyBufferToImage(Buffer* buffer, CommandPool* pool)
{
	CommandBuffer* commandBuffer = pool->beginSingleTimeCommand();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { this->width, this->height, 1 };

	commandBuffer->cmdCopyBufferToImage(buffer->getBuffer(), this->image, this->layout, 1, &region);

	pool->endSingleTimeCommand(commandBuffer);
}

VkImage Image::getImage() const
{
	return this->image;
}

void Image::cleanup()
{
	vkDestroyImage(Instance::get().getDevice(), this->image, nullptr);
}
