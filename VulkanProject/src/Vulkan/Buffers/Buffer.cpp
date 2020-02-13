#include "jaspch.h"

#include "Buffer.h"
#include "Vulkan/Instance.h"


Buffer::Buffer()
{

}

Buffer::~Buffer()
{

}

void Buffer::init(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;

	if (queueFamilyIndices.size() > 1)
		createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	else
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	ERROR_CHECK(vkCreateBuffer(Instance::get().getDevice(), &createInfo, nullptr, &this->buffer), "Failed to create buffer");
}

void Buffer::bindBufferMemory(VkDeviceMemory memory, VkDeviceSize offset)
{
	ERROR_CHECK(vkBindBufferMemory(Instance::get().getDevice(), this->buffer, memory, 0), "Failed to bind memory");
}

VkBuffer Buffer::getBuffer() const
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
