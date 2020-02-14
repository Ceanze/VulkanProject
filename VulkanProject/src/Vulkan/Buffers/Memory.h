#pragma once

#include "jaspch.h"

#include <vulkan/vulkan.h>

typedef uint32_t Offset;

class Buffer;

class Memory
{
public:
	Memory();
	~Memory();

	void bindBuffer(Buffer* buffer);
	void directTransfer(Buffer* buffer, const void* data, uint32_t size, Offset bufferOffset);

	void init(VkMemoryPropertyFlags memProp);

	VkDeviceMemory getMemory();

	void cleanup();
private:
	VkDeviceMemory memory;
	std::unordered_map<Buffer*, Offset> offsets;
	Offset currentOffset;
};