#include "jaspch.h"
#include "VulkanProfiler.h"

#include "Instance.h"
#include "CommandBuffer.h"

#include <imgui.h>

VulkanProfiler& VulkanProfiler::get()
{
	static VulkanProfiler vp;
	return vp;
}

VulkanProfiler::~VulkanProfiler()
{
}

void VulkanProfiler::init(uint32_t timestampCount, uint32_t plotDataCount, float updateFreq)
{
	this->queryCount = timestampCount;
	this->plotDataCount = plotDataCount;
	this->updateFreq = updateFreq;

	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP; // Can be OCCLUSION or PIPELINE_STATISTICS
	createInfo.queryCount = this->queryCount; // How many queries that is managed by the pool
	createInfo.pipelineStatistics = 0;

	ERROR_CHECK(vkCreateQueryPool(Instance::get().getDevice(), &createInfo, nullptr, &this->queryPool), "Failed to create query pool!");

	this->freeIndex = 0;
}

void VulkanProfiler::cleanup()
{
	vkDestroyQueryPool(Instance::get().getDevice(), this->queryPool, nullptr);
}

void VulkanProfiler::render(float dt)
{
	this->timeSinceUpdate += dt;

	float timestampPeriod = Instance::get().getPhysicalDeviceProperties().limits.timestampPeriod;

	// Edit the last element to be new data and move the data
	ImGui::Begin("Timings");
	for (auto& r : this->results) {
		float average = this->averages[r.first];
		if (this->timeSinceUpdate > 1/this->updateFreq) {
			average = 0.0f;
			this->plotResults[r.first][this->plotResults[r.first].size() - 1] = ((r.second.second - r.second.first) * timestampPeriod);
			for (size_t i = 0; i < this->plotResults[r.first].size() - 1; i++) {
				this->plotResults[r.first][i] = this->plotResults[r.first][i + 1];
				average += this->plotResults[r.first][i];
			}
			this->timeSinceUpdate = 0.0f;
			this->averages[r.first] = average;
		}

		average /= this->plotDataCount;
		std::ostringstream overlay;
		overlay << "Average: " << average;

		std::string s = "Timings of " + r.first;
		ImGui::PlotLines(s.c_str(), this->plotResults[r.first].data(), this->plotResults[r.first].size(), 0, overlay.str().c_str(), -1.f, 1.f, {0, 80});
	}
	ImGui::End();
}

void VulkanProfiler::addTimestamp(std::string name)
{
	size_t index = this->freeIndex;
	this->timestamps[name] = Timestamp(index, index + 1);
	this->freeIndex += 2;

	this->plotResults[name].resize(this->plotDataCount);
	this->averages[name] = 0.0f;
}

void VulkanProfiler::startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage)
{
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->queryPool, this->timestamps[name].first);
}

void VulkanProfiler::endTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage)
{
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->queryPool, this->timestamps[name].second);
}

void VulkanProfiler::getTimestamps()
{
	Instance& instance = Instance::get();

	uint32_t timestampValidBits = instance.getQueueProperties(instance.getGraphicsQueue().queueIndex).timestampValidBits;

	// Get _all_ timestamps
	size_t timestampCount = this->timestamps.size() * 2;
	std::vector<uint64_t> poolResults(timestampCount);
	ERROR_CHECK(vkGetQueryPoolResults(instance.getDevice(), this->queryPool, 0,
		timestampCount, poolResults.size() * sizeof(uint64_t), poolResults.data(),
		sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT), "Failed to get timestamps!");

	// If you CRASH here then you might have forgotten to init the necessary number of timestamp count with the
	// amount of timestamps that you have!

	// Save the timestamps
	for (auto& t : this->timestamps) {
		this->results[t.first].first = glm::bitfieldExtract<uint64_t>(poolResults[t.second.first], 0, timestampValidBits);
		this->results[t.first].second = glm::bitfieldExtract<uint64_t>(poolResults[t.second.second], 0, timestampValidBits);
	}
}

void VulkanProfiler::resetTimestampInterval(CommandBuffer* commandBuffer, uint32_t firstQuery, uint32_t queryCount)
{
	commandBuffer->cmdResetQueryPool(this->queryPool, firstQuery, queryCount);
}

void VulkanProfiler::resetTimestamp(const std::string& name, CommandBuffer* commandBuffer)
{
	commandBuffer->cmdResetQueryPool(this->queryPool, this->timestamps[name].first, 2);
}

void VulkanProfiler::resetAll(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdResetQueryPool(this->queryPool, 0, this->timestamps.size() * 2);
}

VulkanProfiler::VulkanProfiler()
	: freeIndex(0), queryPool(VK_NULL_HANDLE), queryCount(0),
	plotDataCount(0), timeSinceUpdate(0.0f), updateFreq(0)
{
}