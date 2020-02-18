#pragma once

#include "vulkan/vulkan.h"

class PushConstants
{
public:
	PushConstants();
	~PushConstants();

	void setLayout(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset);

	// Set pointer to the data. This is the data which will be pushed to the command buffer.
	void setDataPtr(uint32_t size, uint32_t offset, const void* data);

	// Set pointer to the data. This uses the same offset and size when init was called.
	void setDataPtr(const void* data);

	VkPushConstantRange getRange() const;
	const void* getData() const;
	uint32_t getSize() const;
	uint32_t getOffset() const;

private:
	VkPushConstantRange range;
	const void* data{nullptr};
	uint32_t size{0};
	uint32_t offset{ 0 };
};