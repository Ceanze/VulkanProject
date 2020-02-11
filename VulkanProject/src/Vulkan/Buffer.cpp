#include "jaspch.h"

#include "Buffer.h"
#include "Instance.h"


Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;

	if(!queueFamilyIndices.empty())
		createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	else
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	ERROR_CHECK(vkCreateBuffer(Instance::get().getDevice(), &createInfo, nullptr, &this->buffer), "Failed to create buffer");
}

Buffer::~Buffer()
{

}

void Buffer::bindBufferMemory(VkDeviceMemory memory, VkDeviceSize offset)
{
	ERROR_CHECK(vkBindBufferMemory(Instance::get().getDevice(), this->buffer, memory, 0), "Failed to bind memory");
}

VkBuffer Buffer::getbuffer() const
{
	return this->buffer;
}

VkMemoryRequirements Buffer::getMemReq() const
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Instance::get().getDevice(), this->buffer, &memRequirements);

	return memRequirements;
}

void Buffer::cleanup()
{
	vkDestroyBuffer(Instance::get().getDevice(), this->buffer, nullptr);
}
