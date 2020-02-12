#pragma once

#include <vulkan/vulkan.h>

class Buffer;

class Image
{
public:
	Image();
	~Image();

	void init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices);

	void bindImageMemory(VkDeviceMemory memory, VkDeviceSize offset);

	VkImage getImage();

	void cleanup();

private:
	VkImage image;
};