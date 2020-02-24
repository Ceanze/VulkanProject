#include "jaspch.h"
#include "DescriptorManager.h"
#include "../Instance.h"

DescriptorManager::DescriptorManager() : pool(VK_NULL_HANDLE)
{
}

DescriptorManager::~DescriptorManager()
{
}

void DescriptorManager::addLayout(const DescriptorLayout& layout)
{
	this->setLayouts.push_back(layout);
}

void DescriptorManager::init(uint32_t numCopies)
{
	// Create descriptor pool.
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto& layout : this->setLayouts)
	{
		auto sizes = layout.getPoolSizes(numCopies);
		poolSizes.insert(poolSizes.end(), sizes.begin(), sizes.end());
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(numCopies * this->setLayouts.size());

	VkResult result = vkCreateDescriptorPool(Instance::get().getDevice(), &poolInfo, nullptr, &this->pool);
	JAS_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool!");

	// Create descriptor sets.
	for (SetIndex i = 0; i < (SetIndex)this->setLayouts.size(); i++)
	{
		std::vector<VkDescriptorSetLayout> layouts(numCopies, this->setLayouts[i].getLayout()); // Assume only one set is used (only take the first set to create the array of it).
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = this->pool;
		allocInfo.descriptorSetCount = numCopies;
		allocInfo.pSetLayouts = layouts.data();

		this->descriptorSets[i].resize(numCopies);
		VkResult result = vkAllocateDescriptorSets(Instance::get().getDevice(), &allocInfo, this->descriptorSets[i].data());
		JAS_ASSERT(result == VK_SUCCESS, "Failed to allocate descriptor sets!");
	}
}

void DescriptorManager::updateBufferDesc(SetIndex index, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	std::pair<VkDescriptorType, GroupIndex> elem = this->setLayouts[index].getWriteElem(binding);
	
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;

	BufferGroup& group = this->bufferGroups[index][elem.second];
	if (group.infos.empty())
	{
		group.binding = binding;
		group.type = elem.first;
	}
	group.infos.push_back(bufferInfo);
}

void DescriptorManager::updateImageDesc(SetIndex index, uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler)
{
	std::pair<VkDescriptorType, GroupIndex> elem = this->setLayouts[index].getWriteElem(binding);

	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = layout;
	imageInfo.imageView = view;
	imageInfo.sampler = sampler;

	ImageGroup& group = this->imageGroups[index][elem.second];
	if (group.infos.empty())
	{
		group.binding = binding;
		group.type = elem.first;
	}
	group.infos.push_back(imageInfo);
}

void DescriptorManager::updateSets(std::vector<SetIndex> sets, uint32_t copyIndex)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites = {};

	for (SetIndex index : sets)
	{
		auto bufferSet = this->bufferGroups.find(index);
		if (bufferSet != this->bufferGroups.end())
		{
			for (auto& group : bufferSet->second)
			{
				BufferGroup& bufferGroup = group.second;
				VkWriteDescriptorSet desc = {};
				desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				desc.dstSet = this->descriptorSets[index][copyIndex];
				desc.dstArrayElement = 0;
				desc.descriptorType = bufferGroup.type;
				desc.pImageInfo = nullptr;
				desc.dstBinding = bufferGroup.binding;
				desc.descriptorCount = static_cast<uint32_t>(bufferGroup.infos.size());
				desc.pBufferInfo = bufferGroup.infos.data();
				desc.pTexelBufferView = nullptr;
				descriptorWrites.push_back(desc);
			}
		}

		auto imageSet = this->imageGroups.find(index);
		if (imageSet != this->imageGroups.end())
		{
			for (auto& group : imageSet->second)
			{
				ImageGroup& imageGroup = group.second;
				VkWriteDescriptorSet desc = {};
				desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				desc.dstSet = this->descriptorSets[index][copyIndex];
				desc.dstArrayElement = 0;
				desc.descriptorType = imageGroup.type;
				desc.pImageInfo = imageGroup.infos.data();
				desc.dstBinding = imageGroup.binding;
				desc.descriptorCount = static_cast<uint32_t>(imageGroup.infos.size());
				desc.pBufferInfo = nullptr;
				desc.pTexelBufferView = nullptr;
				descriptorWrites.push_back(desc);
			}
		}
	}

	vkUpdateDescriptorSets(Instance::get().getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	this->bufferGroups.clear();
	this->imageGroups.clear();
}

void DescriptorManager::cleanup()
{
	vkDestroyDescriptorPool(Instance::get().getDevice(), this->pool, nullptr);

	for (auto& layout : this->setLayouts)
		vkDestroyDescriptorSetLayout(Instance::get().getDevice(), layout.getLayout(), nullptr);
}

VkDescriptorSet DescriptorManager::getSet(uint32_t copyIndex, SetIndex index) const
{
	auto& it = this->descriptorSets.find(index);
	JAS_ASSERT(it != this->descriptorSets.end(), "Set index out of range!");
	return it->second[copyIndex];
}

std::vector<DescriptorLayout> DescriptorManager::getLayouts() const
{
	return this->setLayouts;
}
