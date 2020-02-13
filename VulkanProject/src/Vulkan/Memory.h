#pragma once

#include <vulkan/vulkan.h>

class Memory
{
public:
	Memory();
	~Memory();

	void init(VkDeviceSize size, uint32_t memoryTypeIndex);

	VkDeviceMemory getMemory();

	void cleanup();
private:
	VkDeviceMemory memory;
};