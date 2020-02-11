#pragma once
#include "jaspch.h"

#include <vulkan/vulkan.h>

class Buffer
{
public:
	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices);
	~Buffer();

	void bindBufferMemory(VkDeviceMemory memory, VkDeviceSize offset);

	VkBuffer getbuffer() const;
	VkMemoryRequirements getMemReq() const;

	void cleanup();

private:
	VkBuffer buffer;
};