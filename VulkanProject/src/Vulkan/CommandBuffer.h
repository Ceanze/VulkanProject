#pragma once

#include <vulkan/vulkan.h>

class PushConstants;
class RenderPass;
class Pipeline;
class Buffer;

class CommandBuffer
{
public:
	CommandBuffer();
	~CommandBuffer();

	void init(VkCommandPool pool);
	void cleanup();
	VkCommandBuffer getCommandBuffer() const { return this->buffer; }
	void setCommandBuffer(VkCommandBuffer buffer);
	void createCommandBuffer(VkCommandBufferLevel level);
	
	// Record command functions
	void begin(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* instanceInfo);
	void cmdBeginRenderPass(RenderPass* renderPass, VkFramebuffer framebuffer, VkExtent2D extent, const std::vector<VkClearValue>& clearValues, VkSubpassContents subpassContents);
	void cmdBindPipeline(Pipeline* pipeline);
	void cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets);
	void cmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
	void cmdPushConstants(Pipeline* pipeline, const PushConstants* pushConstant);
	void cmdPushConstants(Pipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data);
	void cmdBindDescriptorSets(Pipeline* pipeline, uint32_t firstSet, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets);
	void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
	void cmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	void cmdMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkMemoryBarrier> barriers);
	void cmdBufferMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkBufferMemoryBarrier> barriers);
	void cmdImageMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkImageMemoryBarrier> barriers);
	void cmdExecuteCommands(uint32_t bufferCount, const VkCommandBuffer* secondaryBuffers);
	void cmdEndRenderPass();
	void end();

	// Copy commands (used for transfer queue)
	void cmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
	void cmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);

	// Compute/dispatch commands (used for compute queue)
	void cmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

	// Others
	void cmdWriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
	void cmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
	void cmdBeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
	void cmdEndQuery(VkQueryPool queryPool, uint32_t query);

	// Queue acquire and release
	void acquireBuffer(Buffer* buffer, VkAccessFlags dstAccessMask, uint32_t srcQueue, uint32_t dstQueue, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage);
	void releaseBuffer(Buffer* buffer, VkAccessFlags srcAccessMask, uint32_t srcQueue, uint32_t dstQueue, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage);

private:

	VkCommandPool pool;
	VkCommandBuffer buffer;
};