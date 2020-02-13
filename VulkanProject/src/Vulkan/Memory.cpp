#include "jaspch.h"

#include "Memory.h"
#include "Vulkan/Instance.h"

Memory::Memory()
{
}

Memory::~Memory()
{
}

void Memory::init(VkDeviceSize size, uint32_t memoryTypeIndex)
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	ERROR_CHECK_REF(vkAllocateMemory(Instance::get().getDevice(), &allocInfo, nullptr, &this->memory), "Failed to allocate memory!", this);
}

VkDeviceMemory Memory::getMemory()
{
	return this->memory;
}

void Memory::cleanup()
{
	vkFreeMemory(Instance::get().getDevice(), this->memory, nullptr);
}
