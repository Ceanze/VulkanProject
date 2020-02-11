#pragma once

#include <vulkan/vulkan.h>

class Memory
{
public:
	Memory(VkDeviceSize size, uint32_t memoryTypeIndex);
	~Memory();

	VkDeviceMemory getMemory();

	void cleanup();
private:
	VkDeviceMemory memory;
};