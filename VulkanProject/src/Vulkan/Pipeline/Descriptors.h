#pragma once

#include <vulkan/vulkan.h>

#define DESCRIPTOR(name, type) \
	struct name : public VkDescriptorSetLayoutBinding \
	{ \
		name(VkShaderStageFlags stageFlags, uint32_t count, const VkSampler* pImmutableSamplers) \
		{ \
			this->descriptorType = type; \
			this->descriptorCount = count; \
			this->stageFlags = stageFlags; \
			this->pImmutableSamplers = pImmutableSamplers; \
			this->binding = UINT32_MAX; \
		} \
	}; \

DESCRIPTOR(UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
DESCRIPTOR(DynamicUBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
DESCRIPTOR(SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
DESCRIPTOR(DynamicSSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
DESCRIPTOR(IMG, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)