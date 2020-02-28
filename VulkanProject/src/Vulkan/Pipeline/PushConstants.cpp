#include "jaspch.h"
#include "PushConstants.h"

PushConstants::PushConstants() : data(nullptr), offset(0), size(0)
{

}

PushConstants::~PushConstants()
{
	if(this->data) delete this->data;
}

void PushConstants::init()
{
	this->data = (void*)new char[this->size];
}

void PushConstants::addLayout(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
	VkPushConstantRange range;
	range.stageFlags = stageFlags;
	range.size = size;
	range.offset = offset;

	uint32_t newSize = range.size + range.offset;
	if (newSize > this->size) {
		this->size = newSize;
	}

	this->ranges[stageFlags].push_back(range);
}

void PushConstants::setDataPtr(uint32_t size, uint32_t offset, const void* data)
{
	JAS_ASSERT(size+offset > this->size, "Too much data was passed to the push constant!");
	memcpy(this->data, (char*)data + offset, size);
}

void PushConstants::setDataPtr(const void* data)
{
	memcpy(this->data, data, this->size);
}

std::vector<VkPushConstantRange> PushConstants::getRanges() const
{
	std::vector<VkPushConstantRange> allRanges;

	for (auto& rangeVec : this->ranges)
	{
		for (auto range : rangeVec.second)
		{
			allRanges.push_back(range);
		}
	}

	return allRanges;
}

const std::unordered_map<VkShaderStageFlags, std::vector<VkPushConstantRange>>& PushConstants::getRangeMap() const
{
	return this->ranges;
}

const void* PushConstants::getData() const
{
	return this->data;
}

uint32_t PushConstants::getSize() const
{
	return this->size;
}

