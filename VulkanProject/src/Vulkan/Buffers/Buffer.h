#pragma once
#include "jaspch.h"

#include <vulkan/vulkan.h>

class Buffer
{
public:
	Buffer();
	~Buffer();

	void init(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices);

	void bindBufferMemory(VkDeviceMemory memory, VkDeviceSize offset);

	VkBuffer getBuffer() const;
	VkMemoryRequirements getMemReq() const;

	void cleanup();

private:
	VkBuffer buffer;
};