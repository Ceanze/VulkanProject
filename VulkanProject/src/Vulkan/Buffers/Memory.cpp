#include "jaspch.h"

#include "Memory.h"
#include "Vulkan/Instance.h"
#include "Buffer.h"
#include "Vulkan/VulkanCommon.h"
#include "Vulkan/Texture.h"

Memory::Memory() : memory(VK_NULL_HANDLE), currentOffset(0)
{
}

Memory::~Memory()
{
}

void Memory::bindBuffer(Buffer* buffer)
{
	this->bufferOffsets[buffer] = this->currentOffset;
	this->currentOffset += static_cast<Offset>(buffer->getMemReq().size);
}

void Memory::bindTexture(Texture* texture)
{
	this->textureOffsets[texture] = this->currentOffset;
	this->currentOffset += static_cast<Offset>(texture->getMemReq().size);
}

void Memory::directTransfer(Buffer* buffer, const void* data, uint32_t size, Offset bufferOffset)
{
	Offset offset = this->bufferOffsets[buffer];

	void* ptrGpu;
	ERROR_CHECK(vkMapMemory(Instance::get().getDevice(), this->memory, offset + bufferOffset, VK_WHOLE_SIZE, 0, &ptrGpu), "Failed to map memory for buffer!");
	memcpy(ptrGpu, data, size);
	vkUnmapMemory(Instance::get().getDevice(), this->memory);
}

void Memory::init(VkMemoryPropertyFlags memProp)
{
	JAS_ASSERT(this->currentOffset != 0, "No buffers/images bound before allocation of memory!");

	uint32_t typeFilter = 0;
	for (auto buffer : this->bufferOffsets)
		typeFilter |= buffer.first->getMemReq().memoryTypeBits;
	for (auto texture : this->textureOffsets)
		typeFilter |= texture.first->getMemReq().memoryTypeBits;

	uint32_t memoryTypeIndex = findMemoryType(Instance::get().getPhysicalDevice(), typeFilter, memProp);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = this->currentOffset;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	ERROR_CHECK_REF(vkAllocateMemory(Instance::get().getDevice(), &allocInfo, nullptr, &this->memory), "Failed to allocate memory!", this);

	for (auto buffer : this->bufferOffsets)
		ERROR_CHECK(vkBindBufferMemory(Instance::get().getDevice(), buffer.first->getBuffer(), this->memory, buffer.second), "Failed to bind buffer memory!");

	for (auto texture : this->textureOffsets)
		ERROR_CHECK(vkBindImageMemory(Instance::get().getDevice(), texture.first->getVkImage(), this->memory, texture.second), "Failed to bind image memory!");
}

VkDeviceMemory Memory::getMemory()
{
	return this->memory;
}

void Memory::cleanup()
{
	vkFreeMemory(Instance::get().getDevice(), this->memory, nullptr);
}
