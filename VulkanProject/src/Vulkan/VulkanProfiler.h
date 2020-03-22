#pragma once

#include <vulkan/vulkan.h>
#include "jaspch.h"

class CommandBuffer;
class CommandPool;

//typedef std::pair<uint64_t, uint64_t> Timestamp;

class VulkanProfiler
{
public:
	struct Timestamp
	{
		Timestamp() : start(0), end(0), buffer(nullptr), average(0), id(0) {};
		Timestamp(uint64_t start, uint64_t end, CommandBuffer* buffer) : start(start), end(end), buffer(buffer) {};
		uint16_t id;
		uint64_t start;
		uint64_t end;
		float average;
		CommandBuffer* buffer;
	};

	enum class TimeUnit
	{
		NANO = 1,
		MICRO = 1000,
		MILLI = 1000000,
		SECONDS = 1000000000
	};
public:
	static VulkanProfiler& get();
	VulkanProfiler(VulkanProfiler& other) = delete;
	~VulkanProfiler();

	void init(CommandPool* pool, uint32_t plotDataCount, float updateFreq, TimeUnit timeUnit);
	void cleanup();

	const std::unordered_map<std::string, std::vector<Timestamp>>& getResults();

	// Render results using ImGui
	void render(float dt);

	// Specifies how many PAIRS of timestamps to create
	void createTimestamps(uint32_t timestampPairCount);
	void createGraphicsPipelineStats();
	void createComputePipelineStats();
	void addTimestamp(std::string name);
	// Creates timestamp group of several timestamps, great when having multiple buffers
	void addIndexedTimestamps(std::string name, uint32_t count);
	void addIndexedTimestamps(std::string name, uint32_t count, CommandBuffer** buffers);
	void setTimestampBuffer(std::string name, uint32_t index, CommandBuffer* buffer);

	// Starts a measured timestamp, returns a timestamp object to keep track of the current timestamp
	void startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);
	void startIndexedTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t index);
	void startGraphicsPipelineStat(CommandBuffer* commandBuffer);
	void startComputePipelineStat(CommandBuffer* commandBuffer);

	// Ends the selected timestamp using the timestamp object
	void endTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);
	void endIndexedTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t index);
	void endGraphicsPipelineStat(CommandBuffer* commandBuffer);
	void endComputePipelineStat(CommandBuffer* commandBuffer);

	// Should be called after a submit has been done to guarantee to retrive correct timestamps (function does sync). Function also resets all timestamps
	void getBufferTimestamps(CommandBuffer* buffer);
	void getPipelineStats();
	void getAllQueries();

	void resetTimestampInterval(CommandBuffer* commandBuffer, uint32_t firstQuery, uint32_t queryCount);
	void resetTimestamp(const std::string& name, CommandBuffer* commandBuffer);
	void resetIndexedTimestamp(const std::string& name, CommandBuffer* commandBuffer, uint32_t index);
	void resetBufferTimestamps(CommandBuffer* commandBuffer);
	void resetAllTimestamps(CommandBuffer* commandBuffer);
	void resetPipelineStats(CommandBuffer* commandBuffer);
	void resetAll(CommandBuffer* commandBuffer);
	void setupTimers(CommandPool* pool);

private:
	VulkanProfiler();
	void saveResults(std::string filePath);
	std::string getTimeUnitName();

	VkQueryPool timestampQueryPool;
	VkQueryPool graphicsPipelineStatPool;
	VkQueryPool computePipelineStatPool;
	uint32_t timestampCount;
	//uint32_t pipelineStatCount;
	size_t freeIndex;
	size_t plotDataCount;
	float timeSinceUpdate;
	float updateFreq;
	TimeUnit timeUnit;
	std::unordered_map<std::string, std::vector<float>> plotResults;
	std::unordered_map<std::string, std::vector<Timestamp>> results;
	std::unordered_map<std::string, std::vector<Timestamp>> timestamps;
	std::unordered_map<std::string, float> averages;
	std::unordered_map<std::string, float> maxTime;

	uint64_t startTimeCPU;
	uint64_t startTimeGPU;
	std::unordered_map<std::string, std::vector<Timestamp>> timeResults;

	std::vector<uint64_t> graphicsPipelineStat;
	std::vector<uint64_t> computePipelineStat;
	std::vector<std::string> graphicsPipelineStatNames;
};