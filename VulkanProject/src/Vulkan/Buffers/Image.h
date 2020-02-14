#pragma once

#include <vulkan/vulkan.h>

class Buffer;
class CommandPool;

class Image
{
public:
	struct TransistionDesc
	{
		VkFormat format;
		VkImageLayout oldLayout;
		VkImageLayout newLayout;
		CommandPool* pool;
	};

public:
	Image();
	~Image();

	void init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices);

	void transistionLayout(TransistionDesc& desc);
	void copyBufferToImage(Buffer* buffer, CommandPool* pool);

	VkImage getImage() const;
	VkImageLayout getLayout() const { return this->layout; }

	void cleanup();

private:
	VkImage image;
	VkImageLayout layout;

	uint32_t width;
	uint32_t height;
};