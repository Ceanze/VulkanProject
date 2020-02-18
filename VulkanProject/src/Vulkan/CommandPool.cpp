#include "jaspch.h"
#include "CommandPool.h"
#include "VulkanCommon.h"
#include "Vulkan/Instance.h"
#include "CommandBuffer.h"

CommandPool::CommandPool() :
	queueFamily(Queue::GRAPHICS), pool(VK_NULL_HANDLE)
{
}

CommandPool::~CommandPool()
{
}

void CommandPool::init(Queue queueFamily)
{
	this->queueFamily = queueFamily;
	createCommandPool();
}

void CommandPool::cleanup()
{
	vkDestroyCommandPool(Instance::get().getDevice(), this->pool, nullptr);

	// Command buffers are automatically freed when command pool is destroyed, just cleans memory
	for (auto buffer : this->buffers) {
		buffer->cleanup();
		delete buffer;
	}
	this->buffers.clear();
}

QueueVK CommandPool::getQueue() const
{
	switch (this->queueFamily)
	{
	case Queue::GRAPHICS:
		return Instance::get().getGraphicsQueue();
	default:
		JAS_ASSERT(false, "Only graphics queue is currently supported!");
	}
}

CommandBuffer* CommandPool::beginSingleTimeCommand()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;

	CommandBuffer* buffer = createCommandBuffer();

	buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return buffer;
}

void CommandPool::endSingleTimeCommand(CommandBuffer* buffer)
{
	buffer->end();

	VkCommandBuffer commandBuffer = buffer->getCommandBuffer();
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(getQueue().queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(getQueue().queue);

	vkFreeCommandBuffers(Instance::get().getDevice(), this->pool, 1, &commandBuffer);
	removeCommandBuffer(buffer);
}

// Creates one (1) command buffer only
CommandBuffer* CommandPool::createCommandBuffer()
{
	CommandBuffer* b = new CommandBuffer();
	b->init(this->pool);
	b->createCommandBuffer();
	buffers.push_back(b);
	return b;
}

// Used to create several command buffers at a time for optimization
std::vector<CommandBuffer*> CommandPool::createCommandBuffers(uint32_t count)
{
	std::vector<VkCommandBuffer> vkBuffers(count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	ERROR_CHECK(vkAllocateCommandBuffers(Instance::get().getDevice(), &allocInfo, vkBuffers.data()), "Failed to allocate command buffers!")

	std::vector<CommandBuffer*> b;
	for (size_t i = 0; i < count; i++) {
		b[i] = new CommandBuffer;
		b[i]->init(this->pool);
		b[i]->setCommandBuffer(vkBuffers[i]);
	}
	
	return b;
}

void CommandPool::removeCommandBuffer(CommandBuffer* buffer)
{
	buffers.erase(std::remove(buffers.begin(), buffers.end(), buffer), buffers.end());
	delete buffer;
}

void CommandPool::createCommandPool()
{
	// Get queues
	Instance& instance = Instance::get();
	uint32_t index = findQueueIndex((VkQueueFlagBits)queueFamily, instance.getPhysicalDevice());

	// Create pool for queue
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = index;
	poolInfo.flags = 0; // Optional
	poolInfo.pNext = nullptr;

	ERROR_CHECK(vkCreateCommandPool(instance.getDevice(), &poolInfo, nullptr, &this->pool), "Failed to create command pool!");
}
