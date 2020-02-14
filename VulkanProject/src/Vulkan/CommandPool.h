#pragma once

#include "jaspch.h"

#include <vulkan/vulkan.h>

class CommandBuffer;

class CommandPool
{
public:
	enum class Queue {
		GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
		COMPUTE = VK_QUEUE_COMPUTE_BIT,
		TRANSFER = VK_QUEUE_TRANSFER_BIT
	};
public:
	CommandPool();
	~CommandPool();
	
	void init(Queue queueFamily);
	void cleanup();
	Queue getQueueFamily() const { return this->queueFamily; }
	VkQueue getQueue() const;

	CommandBuffer* beginSingleTimeCommand();
	void endSingleTimeCommand(CommandBuffer* buffer);

	CommandBuffer* createCommandBuffer();
	std::vector<CommandBuffer*> createCommandBuffers(uint32_t count);
	void removeCommandBuffer(CommandBuffer* buffer);

private:
	void createCommandPool();

	VkCommandPool pool;
	Queue queueFamily;
	std::vector<CommandBuffer*> buffers;
};