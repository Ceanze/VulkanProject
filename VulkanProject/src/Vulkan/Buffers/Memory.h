#pragma once

#include "jaspch.h"

#include <vulkan/vulkan.h>

typedef uint64_t Offset;

class Buffer;
class Texture;

class Memory
{
public:
	Memory();
	~Memory();

	void bindBuffer(Buffer* buffer);
	void bindTexture(Texture* texture);
	void directTransfer(Buffer* buffer, const void* data, uint64_t size, Offset bufferOffset);

	void init(VkMemoryPropertyFlags memProp);

	VkDeviceMemory getMemory();

	void cleanup();
private:
	VkDeviceMemory memory;
	std::unordered_map<Buffer*, Offset> bufferOffsets;
	std::unordered_map<Texture*, Offset> textureOffsets;
	Offset currentOffset;
};