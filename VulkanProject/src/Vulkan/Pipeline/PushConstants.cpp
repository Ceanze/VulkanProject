#include "jaspch.h"
#include "PushConstants.h"

PushConstants::PushConstants() : data(nullptr), offset(0), size(0)
{
	this->range = {};
}

PushConstants::~PushConstants()
{
}

void PushConstants::setLayout(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
	this->range.stageFlags = stageFlags;
	this->range.size = size;
	this->range.offset = offset;
}

void PushConstants::setDataPtr(uint32_t size, uint32_t offset, const void* data)
{
	JAS_ASSERT(size+offset <= this->range.offset + this->range.size, "Too much data was passed to the push constant!");
	this->data = data;
	this->size = size;
	this->offset = offset;
}

void PushConstants::setDataPtr(const void* data)
{
	this->data = data;
	this->size = this->range.size;
	this->offset = this->range.offset;
}

VkPushConstantRange PushConstants::getRange() const
{
	return this->range;
}

const void* PushConstants::getData() const
{
	return this->data;
}

uint32_t PushConstants::getSize() const
{
	return this->size;
}

uint32_t PushConstants::getOffset() const
{
	return this->offset;
}
