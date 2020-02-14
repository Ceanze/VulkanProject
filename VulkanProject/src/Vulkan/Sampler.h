#pragma once

#include <vulkan/vulkan.h>

class Sampler
{
public:
	Sampler();
	~Sampler();

	void init(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode uWrap, VkSamplerAddressMode vWrap);
	void cleanup();

	VkSampler getSampler() const { return this->sampler; }

private:
	VkSampler sampler;
};