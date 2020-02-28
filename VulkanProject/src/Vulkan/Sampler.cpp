#include "jaspch.h"
#include "Sampler.h"
#include "Vulkan/Instance.h"

Sampler::Sampler() : sampler(VK_NULL_HANDLE)
{
}

Sampler::~Sampler()
{
}

void Sampler::init(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode uWrap, VkSamplerAddressMode vWrap)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = magFilter;
	samplerInfo.minFilter = minFilter;

	samplerInfo.addressModeU = uWrap;
	samplerInfo.addressModeV = vWrap;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // Used for 3D-textures

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16; // TODO: Should check the device properties limit for this value.
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // Color if sampling outside of memory
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	ERROR_CHECK(vkCreateSampler(Instance::get().getDevice(), &samplerInfo, nullptr, &this->sampler), "Failed to create texture sampler!")
}

void Sampler::cleanup()
{
	vkDestroySampler(Instance::get().getDevice(), this->sampler, nullptr);
}