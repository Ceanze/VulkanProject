#include "jaspch.h"
#include "PushConstants.h"

PushConstants::PushConstants() : data(nullptr), offset(0), size(0)
{

}

PushConstants::~PushConstants()
{
	
}

void PushConstants::init()
{
	this->data = new char[this->size];
}

void PushConstants::cleanup()
{
	if (this->data)
	{
		delete[] this->data;
	}
}

void PushConstants::addLayout(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
	auto it = this->ranges.find(stageFlags);
	if (it == this->ranges.end())
	{
		this->ranges[stageFlags] = {};
		this->ranges[stageFlags].offset = UINT32_MAX;
	}

	VkPushConstantRange& range = this->ranges[stageFlags];
	range.stageFlags = stageFlags;
	range.size += size;
	range.offset = offset < range.offset ? offset : range.offset;

	uint32_t newSize = size + offset;
	if (newSize > this->size) {
		this->size = newSize;
	}
}

void PushConstants::setDataPtr(uint32_t size, uint32_t offset, const void* data)
{
	JAS_ASSERT(size+offset > this->size, "Too much data was passed to the push constant!");
	memcpy((void*)(this->data + offset), data, size);
}

void PushConstants::setDataPtr(const void* data)
{
	memcpy((void*)this->data, data, this->size);
}

std::vector<VkPushConstantRange> PushConstants::getRanges() const
{
	std::vector<VkPushConstantRange> allRanges;

	for (auto& range : this->ranges)
		allRanges.push_back(range.second);

	return allRanges;
}

const std::unordered_map<VkShaderStageFlags, VkPushConstantRange>& PushConstants::getRangeMap() const
{
	return this->ranges;
}

const void* PushConstants::getData() const
{
	return (void*)this->data;
}

uint32_t PushConstants::getSize() const
{
	return this->size;
}

