#include "jaspch.h"
#include "VulkanProfiler.h"

#include "Instance.h"
#include "CommandBuffer.h"

#include <imgui.h>
#include <fstream>

VulkanProfiler& VulkanProfiler::get()
{
	static VulkanProfiler vp;
	return vp;
}

VulkanProfiler::~VulkanProfiler()
{
}

void VulkanProfiler::init(uint32_t plotDataCount, float updateFreq, VulkanProfiler::TimeUnit timeUnit)
{
	this->plotDataCount = plotDataCount;
	this->updateFreq = updateFreq;
	this->timeUnit = timeUnit;
}

void VulkanProfiler::cleanup()
{
	saveResults("GPUresults.json");

	if (this->timestampQueryPool != VK_NULL_HANDLE)
		vkDestroyQueryPool(Instance::get().getDevice(), this->timestampQueryPool, nullptr);

	if (this->graphicsPipelineStatPool != VK_NULL_HANDLE)
		vkDestroyQueryPool(Instance::get().getDevice(), this->graphicsPipelineStatPool, nullptr);

	if (this->computePipelineStatPool != VK_NULL_HANDLE)
		vkDestroyQueryPool(Instance::get().getDevice(), this->computePipelineStatPool, nullptr);

	this->averages.clear();
	this->plotResults.clear();
	this->timestamps.clear();
	this->graphicsPipelineStat.clear();
	this->graphicsPipelineStatNames.clear();
	this->computePipelineStat.clear();
}

void VulkanProfiler::render(float dt)
{
	this->timeSinceUpdate += dt;

	float timestampPeriod = Instance::get().getPhysicalDeviceProperties().limits.timestampPeriod;

	ImGui::Begin("Profiling data");

	// Render timestamp results
	if (this->timestampCount != 0 && ImGui::CollapsingHeader("Timestamps"))
	{
		// Edit the last element to be new data and move the data
		for (auto& r : this->results) {
			float average = this->averages[r.first];
			if (this->timeSinceUpdate > 1 / this->updateFreq) {
				average = 0.0f;
				this->plotResults[r.first][this->plotResults[r.first].size() - 1] = ((r.second.end - r.second.start) * timestampPeriod) / (uint32_t)this->timeUnit;
				for (size_t i = 0; i < this->plotResults[r.first].size() - 1; i++) {
					this->plotResults[r.first][i] = this->plotResults[r.first][i + 1];
					average += this->plotResults[r.first][i];
				}
				this->averages[r.first] = average;
			}

			average /= this->plotDataCount;
			std::ostringstream overlay;
			overlay.precision(2);
			overlay << "Average: " << std::fixed << average << getTimeUnitName();

			std::string s = "Timings of " + r.first;
			ImGui::PlotLines(s.c_str(), this->plotResults[r.first].data(), (int)this->plotResults[r.first].size(), 0, overlay.str().c_str(), -1.f, 1.f, { 0, 80 });
		}
	}

	// Render graphics pipeline statistics
	if (this->graphicsPipelineStatPool != VK_NULL_HANDLE && ImGui::CollapsingHeader("Graphics Pipeline Statistics"))
	{
		for (size_t i = 0; i < this->graphicsPipelineStat.size(); i++) {
			std::string caption = this->graphicsPipelineStatNames[i] + ": %d";
			ImGui::BulletText(caption.c_str(), this->graphicsPipelineStat[i]);
		}
	}

	// Render compute pipeline statistics
	if (this->computePipelineStatPool != VK_NULL_HANDLE && ImGui::CollapsingHeader("Compute Pipeline Statistics"))
	{
		ImGui::BulletText("Compute shader invocations:		: %d", this->computePipelineStat[0]);
	}

	ImGui::End();

	if (this->timeSinceUpdate > 1/this->updateFreq)
		this->timeSinceUpdate = 0.0f;
}

void VulkanProfiler::createTimestamps(uint32_t timestampPairCount)
{
	this->timestampCount = timestampPairCount * 2;

	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP; // Can be OCCLUSION or PIPELINE_STATISTICS
	createInfo.queryCount = this->timestampCount; // How many queries that is managed by the pool
	createInfo.pipelineStatistics = 0;

	ERROR_CHECK(vkCreateQueryPool(Instance::get().getDevice(), &createInfo, nullptr, &this->timestampQueryPool), "Failed to create query pool!");

	this->freeIndex = 0;
}

void VulkanProfiler::createGraphicsPipelineStats()
{
	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
	createInfo.flags = 0;
	createInfo.pNext = nullptr;

	// Hard coded because why not
	createInfo.pipelineStatistics =
		VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
		VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
		VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
		VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
		VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
		VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;

	createInfo.queryCount = 6; // Reflects how many pipeline statistics we have

	// VK_QUERY_PIPELINE_STATISTIC_TESSELATION_CONTROL_SHADER_PATCHES_BIT and *_EVALUATION_SHADER_INVOCATIONS_BIT is also available

	ERROR_CHECK(vkCreateQueryPool(Instance::get().getDevice(), &createInfo, nullptr, &this->graphicsPipelineStatPool), "Failed to create graphics query pool!");

	this->graphicsPipelineStat.resize(createInfo.queryCount);
	this->graphicsPipelineStatNames = {
			"Input assembly vertex count        ",
			"Input assembly primitives count    ",
			"Vertex shader invocations          ",
			"Clipping stage primitives processed",
			"Clipping stage primtives output    ",
			"Fragment shader invocations        "
	};
}

void VulkanProfiler::createComputePipelineStats()
{
	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
	createInfo.flags = 0;
	createInfo.pNext = nullptr;

	// Hard coded because why not
	createInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
	createInfo.queryCount = 1; // Reflects how many pipeline statistics we have

	ERROR_CHECK(vkCreateQueryPool(Instance::get().getDevice(), &createInfo, nullptr, &this->computePipelineStatPool), "Failed to create compute query pool!");

	this->computePipelineStat.resize(createInfo.queryCount);
}

void VulkanProfiler::addTimestamp(std::string name)
{
	size_t index = this->freeIndex;
	this->timestamps[name] = Timestamp(index, index + 1, nullptr);
	this->freeIndex += 2;

	this->plotResults[name].resize(this->plotDataCount);
	this->averages[name] = 0.0f;
}

void VulkanProfiler::addIndexedTimestamps(std::string name, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
	{
		std::string newName = name + std::to_string(i);
		size_t index = this->freeIndex;
		this->timestamps[newName] = Timestamp(index, index + 1, nullptr);
		this->freeIndex += 2;

		this->plotResults[newName].resize(this->plotDataCount);
		this->averages[newName] = 0.0f;
	}
}

void VulkanProfiler::addIndexedTimestamps(std::string name, uint32_t count, CommandBuffer** buffers)
{
	addIndexedTimestamps(name, count);
	for (uint32_t i = 0; i < count; i++) {
		std::string newName = name + std::to_string(i);
		this->timestamps[newName].buffer = buffers[i];
	}
}

void VulkanProfiler::startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage)
{
	this->timestamps[name].buffer = commandBuffer;
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[name].start);
}

void VulkanProfiler::startIndexedTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t index)
{
	std::string newName = name + std::to_string(index);
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[newName].start);
}

void VulkanProfiler::startComputePipelineStat(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdBeginQuery(this->computePipelineStatPool, 0, 0);
}

void VulkanProfiler::startGraphicsPipelineStat(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdBeginQuery(this->graphicsPipelineStatPool, 0, 0);
}

void VulkanProfiler::endTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage)
{
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[name].end);
}

void VulkanProfiler::endIndexedTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t index)
{
	std::string newName = name + std::to_string(index);
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[newName].end);
}

void VulkanProfiler::endComputePipelineStat(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdEndQuery(this->computePipelineStatPool, 0);
}

void VulkanProfiler::endGraphicsPipelineStat(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdEndQuery(this->graphicsPipelineStatPool, 0);
}

void VulkanProfiler::getTimestamps()
{
	if (this->timestampCount == 0)
		return;

	Instance& instance = Instance::get();

	uint32_t timestampValidBits = instance.getQueueProperties(instance.getGraphicsQueue().queueIndex).timestampValidBits;

	// Get _all_ timestamps
	size_t timestampCount = this->timestamps.size() * 2;
	std::vector<uint64_t> poolResults(timestampCount + 1);
	VkResult res = vkGetQueryPoolResults(instance.getDevice(), this->timestampQueryPool, 0,
		(uint32_t)timestampCount, (uint32_t)(poolResults.size() * sizeof(uint64_t)), poolResults.data(),
		sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

	// If you CRASH here then you might have forgotten to init the necessary number of timestamp count with the
	// amount of timestamps that you have!

	// Save the timestamps
	for (auto& t : this->timestamps) {
		this->results[t.first].start = glm::bitfieldExtract<uint64_t>(poolResults[t.second.start], 0, timestampValidBits);
		this->results[t.first].end = glm::bitfieldExtract<uint64_t>(poolResults[t.second.end], 0, timestampValidBits);
	}
}

void VulkanProfiler::getBufferTimestamps(CommandBuffer* buffer)
{
	Instance& instance = Instance::get();
	uint32_t timestampValidBits = instance.getQueueProperties(instance.getGraphicsQueue().queueIndex).timestampValidBits;

	// Get the timestamps that we will get
	size_t timestampCount = 2;
	std::vector<uint64_t> poolResults(timestampCount + 1);
	for (auto& timestamp : this->timestamps) {
		if (timestamp.second.buffer == buffer) {
			VkResult res = vkGetQueryPoolResults(instance.getDevice(), this->timestampQueryPool, timestamp.second.start,
				(uint32_t)timestampCount, (uint32_t)(poolResults.size() * sizeof(uint64_t)), poolResults.data(),
				sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

			if (res == VK_SUCCESS) {
				this->results[timestamp.first].start = glm::bitfieldExtract<uint64_t>(poolResults[0], 0, timestampValidBits);
				this->results[timestamp.first].end = glm::bitfieldExtract<uint64_t>(poolResults[1], 0, timestampValidBits);
			}
		}
	}
}

void VulkanProfiler::getPipelineStats()
{
	if (this->graphicsPipelineStatPool != VK_NULL_HANDLE)
		vkGetQueryPoolResults(Instance::get().getDevice(), this->graphicsPipelineStatPool, 0, 1,
			this->graphicsPipelineStat.size() * sizeof(uint64_t), this->graphicsPipelineStat.data(),
			sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

	if (this->computePipelineStatPool != VK_NULL_HANDLE)
		vkGetQueryPoolResults(Instance::get().getDevice(), this->computePipelineStatPool, 0, 1,
			this->computePipelineStat.size() * sizeof(uint64_t), this->computePipelineStat.data(),
			sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
}

void VulkanProfiler::getAllQueries()
{
	getTimestamps();
	getPipelineStats();
}

void VulkanProfiler::resetTimestampInterval(CommandBuffer* commandBuffer, uint32_t firstQuery, uint32_t queryCount)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, firstQuery, queryCount);
}

void VulkanProfiler::resetTimestamp(const std::string& name, CommandBuffer* commandBuffer)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, (uint32_t)this->timestamps[name].start, 2);
}

void VulkanProfiler::resetIndexedTimestamp(const std::string& name, CommandBuffer* commandBuffer, uint32_t index)
{
	std::string newName = name + std::to_string(index);
	if (this->timestampQueryPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, (uint32_t)this->timestamps[newName].end, 2);
}

void VulkanProfiler::resetBufferTimestamps(CommandBuffer* commandBuffer)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE) {
		for (auto& timestamp : this->timestamps) {
			if (timestamp.second.buffer == commandBuffer)
				commandBuffer->cmdResetQueryPool(this->timestampQueryPool, (uint32_t)timestamp.second.start, 2);
		}
	}
}

void VulkanProfiler::resetAllTimestamps(CommandBuffer* commandBuffer)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, 0, (uint32_t)this->timestamps.size() * 2);
}

void VulkanProfiler::resetPipelineStats(CommandBuffer* commandBuffer)
{
	if (this->graphicsPipelineStatPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->graphicsPipelineStatPool, 0, 1);

	if (this->computePipelineStatPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->computePipelineStatPool, 0, 1);
}

void VulkanProfiler::resetAll(CommandBuffer* commandBuffer)
{
	resetAllTimestamps(commandBuffer);
	resetPipelineStats(commandBuffer);
}

VulkanProfiler::VulkanProfiler()
	: freeIndex(0), timestampQueryPool(VK_NULL_HANDLE), timestampCount(0),
	plotDataCount(0), timeSinceUpdate(0.0f), updateFreq(0), graphicsPipelineStatPool(VK_NULL_HANDLE),
	computePipelineStatPool(VK_NULL_HANDLE), timeUnit(TimeUnit::MILLI)
{
}

void VulkanProfiler::saveResults(std::string filePath)
{
	//((r.second.second - r.second.first) * timestampPeriod) / (uint32_t)this->timeUnit;
	float timestampPeriod = Instance::get().getPhysicalDeviceProperties().limits.timestampPeriod;

	std::ofstream file;
	file.open(filePath);
	file << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
	file.flush();

	uint32_t i = 0;
	for (auto& res : this->results)
	{
		if (i > 0) file << ",";

		std::string name = res.first;
		std::replace(name.begin(), name.end(), '"', '\'');

		file << "{";
		file << "\"name\": \"" << name << "\",";
		file << "\"cat\": \"function\",";
		file << "\"ph\": \"X\",";
		file << "\"pid\": 0,";
		file << "\"tid\": " << i << ",";
		file << "\"ts\": " << (res.second.start * timestampPeriod) / (uint32_t)this->timeUnit << ",";
		file << "\"dur\": " << ((res.second.end - res.second.start) * timestampPeriod) / (uint32_t)this->timeUnit;
		file << "}";
		
		i++;
	}

	file << "]" << std::endl << "}";
	file.flush();
	file.close();
}

std::string VulkanProfiler::getTimeUnitName()
{
	std::string buf;
	switch (this->timeUnit) {
	case TimeUnit::MICRO:
		buf = "us";
		break;
	case TimeUnit::MILLI:
		buf = "ms";
		break;
	case TimeUnit::NANO:
		buf = "ns";
		break;
	case TimeUnit::SECONDS:
		buf = "s";
		break;
	}
	return buf;
}
