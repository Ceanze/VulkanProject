#pragma once
#include "jaspch.h"

#include <vulkan/vulkan.h>

class Memory;

class Buffer
{
public:
	Buffer();
	~Buffer();

	void init(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices);

	VkBuffer getBuffer() const;
	VkMemoryRequirements getMemReq() const;
	VkDeviceSize getSize() const;

	void cleanup();

private:
	VkBuffer buffer;
	VkDeviceSize size;
};