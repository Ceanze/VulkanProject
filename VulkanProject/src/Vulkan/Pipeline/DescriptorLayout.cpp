#include "jaspch.h"
#include "DescriptorLayout.h"
#include "../Instance.h"

DescriptorLayout::DescriptorLayout() : currentBinding(0), layout(VK_NULL_HANDLE)
{
}

DescriptorLayout::~DescriptorLayout()
{
}

void DescriptorLayout::add(VkDescriptorSetLayoutBinding* descriptor)
{
	descriptor->binding = this->currentBinding++;
	this->descriptors.push_back(*descriptor);

	// Create new write group if there are non.
	if (this->writeGroups.empty())
	{
		std::vector<VkDescriptorSetLayoutBinding> newWriteGroup;
		newWriteGroup.push_back(*descriptor);
		this->writeGroups.push_back(newWriteGroup);
	}
	else
	{
		// Check if previous type is different from the new descriptor's type.
		VkDescriptorType previousType = this->writeGroups.back().back().descriptorType;
		if (previousType == descriptor->descriptorType)
			this->writeGroups.back().push_back(*descriptor);
		else
		{
			// Create new write group if type was different.
			std::vector<VkDescriptorSetLayoutBinding> newWriteGroup;
			newWriteGroup.push_back(*descriptor);
			this->writeGroups.push_back(newWriteGroup);
		}
	}
	
	delete descriptor;
}

void DescriptorLayout::init()
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(this->descriptors.size());
	layoutInfo.pBindings = this->descriptors.data();

	VkResult result = vkCreateDescriptorSetLayout(Instance::get().getDevice(), &layoutInfo, nullptr, &this->layout);
	JAS_ASSERT(result == VK_SUCCESS, "Failed to create descriptor set layout!");
}

VkDescriptorSetLayout DescriptorLayout::getLayout() const
{
	return this->layout;
}

std::vector<VkDescriptorPoolSize> DescriptorLayout::getPoolSizes(uint32_t factor) const
{
	std::vector<VkDescriptorPoolSize> result;
	for (auto& group : this->writeGroups)
	{
		VkDescriptorPoolSize size;
		size.descriptorCount = factor * static_cast<uint32_t>(group.size());
		size.type = group.front().descriptorType;
		result.push_back(size);
	}

	return result;
}

std::pair<VkDescriptorType, GroupIndex> DescriptorLayout::getWriteElem(uint32_t binding) const
{
	for (size_t i = 0; i < this->writeGroups.size(); i++)
	{
		auto& lastElement = this->writeGroups[i].back();
		if (lastElement.binding >= binding)
			return std::make_pair(lastElement.descriptorType, i);
	}

	JAS_ASSERT(false, "Binding out of range!");
}
