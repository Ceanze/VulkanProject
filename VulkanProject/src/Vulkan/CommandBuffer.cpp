#include "jaspch.h"
#include "CommandBuffer.h"
#include "Vulkan/Instance.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/PushConstants.h"

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

void CommandBuffer::createCommandBuffer(VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = 1;
	allocInfo.pNext = nullptr;

	ERROR_CHECK(vkAllocateCommandBuffers(Instance::get().getDevice(), &allocInfo, &this->buffer), "Failed to allocate command buffer!");
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* instanceInfo)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags; // Optional
	beginInfo.pInheritanceInfo = instanceInfo; // Optional

	ERROR_CHECK(vkBeginCommandBuffer(this->buffer, &beginInfo), "Failed to begin recording command buffer!");
}

void CommandBuffer::cmdBeginRenderPass(RenderPass* renderPass, VkFramebuffer framebuffer, VkExtent2D extent, const std::vector<VkClearValue>& clearValues, VkSubpassContents subpassContents)
{	
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->getRenderPass();
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	renderPassInfo.pNext = nullptr;

	vkCmdBeginRenderPass(this->buffer, &renderPassInfo, subpassContents);
}

void CommandBuffer::cmdBindPipeline(Pipeline* pipeline)
{
	vkCmdBindPipeline(this->buffer, (VkPipelineBindPoint)pipeline->getType(), pipeline->getPipeline());
}

void CommandBuffer::cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets)
{
	vkCmdBindVertexBuffers(this->buffer, firstBinding, bindingCount, buffers, offsets);
}

void CommandBuffer::cmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
	vkCmdBindIndexBuffer(this->buffer, buffer, offset, indexType);
}

void CommandBuffer::cmdPushConstants(Pipeline* pipeline, const PushConstants* pushConstant)
{
	for (auto& rangeVec : pushConstant->getRangeMap())
	{
		uint32_t minOffset = UINT32_MAX;
		uint32_t maxSize = 0;
		void* data = nullptr;

		for (auto& range : rangeVec.second)
		{
			minOffset = (minOffset > range.offset) ? range.offset : minOffset;
			maxSize = (maxSize < range.offset + range.size) ? range.offset + range.size : maxSize;
		}

		vkCmdPushConstants(this->buffer, pipeline->getPipelineLayout(), rangeVec.first, minOffset, maxSize - minOffset, (void*)((char*)pushConstant->getData() + minOffset));
	}
}

void CommandBuffer::cmdBindDescriptorSets(Pipeline* pipeline, uint32_t firstSet, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets)
{
	vkCmdBindDescriptorSets(this->buffer, (VkPipelineBindPoint)pipeline->getType(),
		pipeline->getPipelineLayout(), 0, static_cast<uint32_t>(sets.size()), sets.data(), static_cast<uint32_t>(offsets.size()), offsets.data());
}

void CommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(this->buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed(this->buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::cmdMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkMemoryBarrier> barriers)
{
	vkCmdPipelineBarrier(
		this->buffer,
		srcStageMask,
		dstStageMask,
		dependencyFlag,
		(uint32_t)barriers.size(), barriers.data(),
		0, nullptr,
		0, nullptr);
}

void CommandBuffer::cmdBufferMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkBufferMemoryBarrier>barriers)
{
	vkCmdPipelineBarrier(this->buffer,
		srcStageMask,
		dstStageMask,
		dependencyFlag,
		0, nullptr,
		(uint32_t)barriers.size(), barriers.data(),
		0, nullptr);
}

void CommandBuffer::cmdImageMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkImageMemoryBarrier> barriers)
{
	vkCmdPipelineBarrier(this->buffer,
		srcStageMask,
		dstStageMask,
		dependencyFlag,
		0, nullptr,
		0, nullptr,
		(uint32_t)barriers.size(), barriers.data());
}

void CommandBuffer::cmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	vkCmdDispatch(this->buffer, groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::cmdExecuteCommands(uint32_t bufferCount, const VkCommandBuffer* secondaryBuffers)
{
	vkCmdExecuteCommands(this->buffer, bufferCount, secondaryBuffers);
}

void CommandBuffer::cmdEndRenderPass()
{
	vkCmdEndRenderPass(this->buffer);
}

void CommandBuffer::end()
{
	ERROR_CHECK(vkEndCommandBuffer(this->buffer), "Failed to record command buffer!")
}

void CommandBuffer::cmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
	vkCmdCopyBuffer(this->buffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void CommandBuffer::cmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
	vkCmdCopyBufferToImage(this->buffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

void CommandBuffer::cmdWriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
	vkCmdWriteTimestamp(this->buffer, pipelineStage, queryPool, query);
}

void CommandBuffer::cmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
	vkCmdResetQueryPool(this->buffer, queryPool, firstQuery, queryCount);
}

void CommandBuffer::cmdBeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(this->buffer, queryPool, query, flags);
}

void CommandBuffer::cmdEndQuery(VkQueryPool queryPool, uint32_t query)
{
	vkCmdEndQuery(this->buffer, queryPool, query);
}
