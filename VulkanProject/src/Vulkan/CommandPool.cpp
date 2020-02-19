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

void CommandPool::init(Queue queueFamily, VkCommandPoolCreateFlags flags)
{
	this->queueFamily = queueFamily;
	createCommandPool(flags);
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
	case Queue::TRANSFER:
		return Instance::get().getTransferQueue();
	case Queue::COMPUTE:
		return Instance::get().getComputeQueue();
	}
}

CommandBuffer* CommandPool::beginSingleTimeCommand()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;

	CommandBuffer* buffer = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

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
CommandBuffer* CommandPool::createCommandBuffer(VkCommandBufferLevel level)
{
	CommandBuffer* b = new CommandBuffer();
	b->init(this->pool);
	b->createCommandBuffer(level);
	buffers.push_back(b);
	return b;
}

// Used to create several command buffers at a time for optimization
std::vector<CommandBuffer*> CommandPool::createCommandBuffers(uint32_t count, VkCommandBufferLevel level)
{
	std::vector<VkCommandBuffer> vkBuffers(count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = count;

	ERROR_CHECK(vkAllocateCommandBuffers(Instance::get().getDevice(), &allocInfo, vkBuffers.data()), "Failed to allocate command buffers!")

	std::vector<CommandBuffer*> b(count);
	for (size_t i = 0; i < count; i++) {
		b[i] = new CommandBuffer;
		b[i]->init(this->pool);
		b[i]->setCommandBuffer(vkBuffers[i]);
		buffers.push_back(b[i]);
	}
	
	return b;
}

void CommandPool::removeCommandBuffer(CommandBuffer* buffer)
{
	buffers.erase(std::remove(buffers.begin(), buffers.end(), buffer), buffers.end());
	delete buffer;
}

void CommandPool::createCommandPool(VkCommandPoolCreateFlags flags)
{
	// Get queues
	Instance& instance = Instance::get();
	uint32_t index = findQueueIndex((VkQueueFlagBits)queueFamily, instance.getPhysicalDevice());

	// Create pool for queue
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = index;
	poolInfo.flags = flags; // Optional
	poolInfo.pNext = nullptr;

	ERROR_CHECK(vkCreateCommandPool(instance.getDevice(), &poolInfo, nullptr, &this->pool), "Failed to create command pool!");
}
