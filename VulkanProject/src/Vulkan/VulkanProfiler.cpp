#include "jaspch.h"
#include "VulkanProfiler.h"

#include "Instance.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Core/CPUProfiler.h"

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

void VulkanProfiler::init(CommandPool* pool, uint32_t plotDataCount, float updateFreq, VulkanProfiler::TimeUnit timeUnit)
{
	this->plotDataCount = plotDataCount;
	this->updateFreq = updateFreq;
	this->timeUnit = timeUnit;

	setupTimers(pool);
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

	this->plotResults.clear();
	this->timestamps.clear();
	this->graphicsPipelineStat.clear();
	this->graphicsPipelineStatNames.clear();
	this->computePipelineStat.clear();
}

const std::unordered_map<std::string, std::vector<VulkanProfiler::Timestamp>>& VulkanProfiler::getResults()
{
	return this->timeResults;
}

void VulkanProfiler::render(float dt)
{
	this->timeSinceUpdate += dt;

	float timestampPeriod = Instance::get().getPhysicalDeviceProperties().limits.timestampPeriod;

	ImGui::Begin("Profiling data");

	// ((r.second[i].end - r.second[i].start) * timestampPeriod) / (uint32_t)this->timeUnit

	// Render timestamp results
	if (this->timestampCount != 0 && ImGui::CollapsingHeader("Timestamps"))
	{
		// Edit the last element to be new data and move the data
		for (auto& r : this->results) {
			float average = this->averages[r.first];
			if (this->timeSinceUpdate > 1 / this->updateFreq) {
				average = 0.0f;
				for (uint32_t i = 0; i < r.second.size(); i++) {
					average += (r.second[i].end - r.second[i].start);
				}
				average = average / r.second.size();
				this->plotResults[r.first].erase(this->plotResults[r.first].begin());
				this->plotResults[r.first].push_back(average);

				if (average > this->maxTime[r.first])
					this->maxTime[r.first] = average;

				average = 0.0f;
				for (uint32_t i = 0; i < this->plotDataCount; i++) {
					average += this->plotResults[r.first][i];
				}
				this->averages[r.first] = average;
			}

			average /= (this->plotDataCount);

			//if (average > 10000000)
			//	JAS_FATAL("OJ");

			std::ostringstream overlay;
			overlay.precision(2);
			overlay << "Average: " << std::fixed << average << getTimeUnitName();

			std::string s = "Timings of " + r.first + " (" + std::to_string(r.second.size()) + " buffers)";
			ImGui::PlotLines(s.c_str(), this->plotResults[r.first].data(), (int)this->plotResults[r.first].size(), 0, overlay.str().c_str(), 0.f, this->maxTime[r.first], { 0, 80 });
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
	this->timestamps[name].push_back(VulkanProfiler::Timestamp(index, index + 1, nullptr));
	this->freeIndex += 2;

	this->plotResults[name].resize(this->plotDataCount);
	this->results[name].push_back(VulkanProfiler::Timestamp());
	this->averages[name] = 0.f;
	this->maxTime[name] = 0.f;
}

void VulkanProfiler::addIndexedTimestamps(std::string name, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
	{
		size_t index = this->freeIndex;
		this->timestamps[name].push_back(Timestamp(index, index + 1, nullptr));
		this->freeIndex += 2;

		this->plotResults[name].resize(this->plotDataCount);
	}
	this->results[name].resize(count);
	this->averages[name] = 0.f;
	this->maxTime[name] = 0.f;
}

void VulkanProfiler::addIndexedTimestamps(std::string name, uint32_t count, CommandBuffer** buffers)
{
	addIndexedTimestamps(name, count);
	for (uint32_t i = 0; i < count; i++) {
		this->timestamps[name][i].buffer = buffers[i];
	}
}

void VulkanProfiler::setTimestampBuffer(std::string name, uint32_t index, CommandBuffer* buffer)
{
	this->timestamps[name][index].buffer = buffer;
}

void VulkanProfiler::startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage)
{
	this->timestamps[name][0].buffer = commandBuffer;
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[name][0].start);
}

void VulkanProfiler::startIndexedTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t index)
{
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[name][index].start);
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
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[name][0].end);
}

void VulkanProfiler::endIndexedTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t index)
{
	commandBuffer->cmdWriteTimestamp(pipelineStage, this->timestampQueryPool, (uint32_t)this->timestamps[name][index].end);
}

void VulkanProfiler::endComputePipelineStat(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdEndQuery(this->computePipelineStatPool, 0);
}

void VulkanProfiler::endGraphicsPipelineStat(CommandBuffer* commandBuffer)
{
	commandBuffer->cmdEndQuery(this->graphicsPipelineStatPool, 0);
}

void VulkanProfiler::getBufferTimestamps(CommandBuffer* buffer)
{
	Instance& instance = Instance::get();
	double timestampPeriod = instance.getPhysicalDeviceProperties().limits.timestampPeriod;
	uint32_t timestampValidBits = instance.getQueueProperties(instance.getGraphicsQueue().queueIndex).timestampValidBits;

	// Get the timestamps that we will get
	size_t timestampCount = 2;
	std::vector<uint64_t> poolResults(timestampCount + 1);
	for (auto& timestamp : this->timestamps) {
		for (uint32_t i = 0; i < timestamp.second.size(); i++)
		{
			if (timestamp.second[i].buffer == buffer) {
				VkResult res = vkGetQueryPoolResults(instance.getDevice(), this->timestampQueryPool, timestamp.second[i].start,
					(uint32_t)timestampCount, (uint32_t)(poolResults.size() * sizeof(uint64_t)), poolResults.data(),
					sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

				if (res == VK_SUCCESS) {
					this->results[timestamp.first][i].start = glm::bitfieldExtract<uint64_t>(poolResults[0], 0, timestampValidBits);
					this->results[timestamp.first][i].end = glm::bitfieldExtract<uint64_t>(poolResults[1], 0, timestampValidBits);

					this->results[timestamp.first][i].start = (((this->results[timestamp.first][i].start - this->startTimeGPU) * timestampPeriod) / (uint64_t)this->timeUnit);
					this->results[timestamp.first][i].end = (((this->results[timestamp.first][i].end - this->startTimeGPU) * timestampPeriod) / (uint64_t)this->timeUnit);
					this->results[timestamp.first][i].id = i;

					if (this->timeResults[timestamp.first].size() == this->plotDataCount) {
						this->timeResults[timestamp.first].erase(this->timeResults[timestamp.first].begin());
					}
					this->timeResults[timestamp.first].push_back(this->results[timestamp.first][i]);

					//auto it = this->startTimes.find(timestamp.first);
					//if (it == this->startTimes.end()) {
					//	this->startTimes[timestamp.first].resize(timestamp.second.size());
					//}

					//if (this->startTimes[timestamp.first][i] == 0) {
					//	this->startTimes[timestamp.first][i] = this->results[timestamp.first][i].start;
					//}
				}
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
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, (uint32_t)this->timestamps[name][0].start, 2);
}

void VulkanProfiler::resetIndexedTimestamp(const std::string& name, CommandBuffer* commandBuffer, uint32_t index)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, (uint32_t)this->timestamps[name][index].end, 2);
}

void VulkanProfiler::resetBufferTimestamps(CommandBuffer* commandBuffer)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE) {
		for (auto& timestamp : this->timestamps) {
			for (uint32_t i = 0; i < timestamp.second.size(); i++) {
				if (timestamp.second[i].buffer == commandBuffer)
					commandBuffer->cmdResetQueryPool(this->timestampQueryPool, (uint32_t)timestamp.second[i].start, 2);
			}
		}
	}
}

void VulkanProfiler::resetAllTimestamps(CommandBuffer* commandBuffer)
{
	if (this->timestampQueryPool != VK_NULL_HANDLE)
		commandBuffer->cmdResetQueryPool(this->timestampQueryPool, 0, this->timestampCount);
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

	uint32_t j = 0;
	for (auto& res : this->results)
	{
		for (uint32_t i = 0; i < res.second.size(); i++, j++)
		{
			if (j > 0) file << ",";

			std::string name = res.first;
			std::replace(name.begin(), name.end(), '"', '\'');

			file << "{";
			file << "\"name\": \"" << name << " " << i << "\",";
			file << "\"cat\": \"function\",";
			file << "\"ph\": \"X\",";
			file << "\"pid\": " << 1 << ",";
			file << "\"tid\": " << 0 << ",";
			file << "\"ts\": " << res.second[i].start << ",";
			file << "\"dur\": " << (res.second[i].end - res.second[i].start);
			file << "}";
		}
	}

	file << "]" << std::endl << "}";
	file.flush();
	file.close();
}

void VulkanProfiler::setupTimers(CommandPool* pool)
{
	// Create temp query pool
	VkQueryPool qPool;
	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP; // Can be OCCLUSION or PIPELINE_STATISTICS
	createInfo.queryCount = 1; // How many queries that is managed by the pool
	createInfo.pipelineStatistics = 0;
	ERROR_CHECK(vkCreateQueryPool(Instance::get().getDevice(), &createInfo, nullptr, &qPool), "Failed to create query pool!");

	// Create event to signal
	VkEventCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	VkEvent e;
	ERROR_CHECK(vkCreateEvent(Instance::get().getDevice(), &info, nullptr, &e), "Failed to create event for vulkan profiler!");

	// Create single time command buffer
	CommandBuffer* buffer = pool->beginSingleTimeCommand();
	buffer->cmdResetQueryPool(qPool, 0, 1);
	vkCmdWaitEvents(buffer->getCommandBuffer(), 1, &e, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0, nullptr, 0, nullptr, 0, nullptr);
	buffer->cmdWriteTimestamp(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, qPool, 0);
	buffer->end();
	
	VkSubmitInfo sInfo = {};
	sInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	sInfo.commandBufferCount = 1;
	VkCommandBuffer cb = buffer->getCommandBuffer();
	sInfo.pCommandBuffers = &cb;

	vkQueueSubmit(Instance::get().getGraphicsQueue().queue, 1, &sInfo, VK_NULL_HANDLE);

	// Ensure that command buffer is at the wait event so that the timestamp will execute right after it to sync with host
	std::this_thread::sleep_for(std::chrono::seconds(1));

	ERROR_CHECK(vkSetEvent(Instance::get().getDevice(), e), "Failed to set event for profiler!");
	//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	this->startTimeCPU = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();

	ERROR_CHECK(vkGetQueryPoolResults(Instance::get().getDevice(), qPool, 0,
		1u, sizeof(uint64_t), &this->startTimeGPU, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT), "Failed to get first timestamp!");

	// Fence might be needed to ensure that the result has been written

	// Cleanup
	vkQueueWaitIdle(Instance::get().getGraphicsQueue().queue);
	vkDestroyQueryPool(Instance::get().getDevice(), qPool, nullptr);
	vkDestroyEvent(Instance::get().getDevice(), e, nullptr);
	pool->removeCommandBuffer(buffer);

	Instrumentation::get().setStartTime(this->startTimeCPU);
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
