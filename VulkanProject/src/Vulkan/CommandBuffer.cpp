#include "jaspch.h"
#include "CommandBuffer.h"
#include "Vulkan/Instance.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/Pipeline.h"

CommandBuffer::CommandBuffer() :
	pool(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE)
{
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::init(VkCommandPool pool)
{
	this->pool = pool;
}

void CommandBuffer::cleanup()
{
	// Commands buffers are cleared automatically by the command pool
}

void CommandBuffer::setCommandBuffer(VkCommandBuffer buffer)
{
	this->buffer = buffer;
}

void CommandBuffer::createCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	allocInfo.pNext = nullptr;

	ERROR_CHECK(vkAllocateCommandBuffers(Instance::get().getDevice(), &allocInfo, &this->buffer), "Failed to allocate command buffer!");
}

void CommandBuffer::begin()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	ERROR_CHECK(vkBeginCommandBuffer(this->buffer, &beginInfo), "Failed to begin recording command buffer!");
}

void CommandBuffer::cmdBeginRenderPass(RenderPass* renderPass, VkFramebuffer framebuffer, VkExtent2D extent, VkClearValue clearColor)
{	
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->getRenderPass();
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;
	renderPassInfo.pNext = nullptr;

	vkCmdBeginRenderPass(this->buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::cmdBindPipeline(Pipeline* pipeline)
{
	vkCmdBindPipeline(this->buffer, (VkPipelineBindPoint)pipeline->getType(), pipeline->getPipeline());
}

void CommandBuffer::cmdBindDescriptorSets(Pipeline* pipeline, uint32_t firstSet, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets)
{
	vkCmdBindDescriptorSets(this->buffer, (VkPipelineBindPoint)pipeline->getType(),
		pipeline->getPipelineLayout(), 0, sets.size(), sets.data(), offsets.size(), offsets.data());
}

void CommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(this->buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed(this->buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::cmdEndRenderPass()
{
	vkCmdEndRenderPass(this->buffer);
}

void CommandBuffer::end()
{
	ERROR_CHECK(vkEndCommandBuffer(this->buffer), "Failed to record command buffer!")
}
