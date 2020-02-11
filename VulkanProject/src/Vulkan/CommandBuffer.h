#pragma once

#include <vulkan/vulkan.h>

class RenderPass;
class Framebuffer;
class Pipeline;

class CommandBuffer
{
public:
	CommandBuffer();
	~CommandBuffer();

	void init(VkCommandPool pool);
	void cleanup();
	VkCommandBuffer getCommandBuffer() const { return this->buffer; }
	void setCommandBuffer(VkCommandBuffer buffer);
	void createCommandBuffer();
	
	// Record command functions
	void begin();
	void cmdBeginRenderPass(RenderPass* renderPass, Framebuffer* framebuffer, VkExtent2D extent, VkClearValue clearColor);
	void cmdBindPipeline(Pipeline* pipeline);
	void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
	void cmdEndRenderPass();
	void end();

private:

	VkCommandPool pool;
	VkCommandBuffer buffer;
};