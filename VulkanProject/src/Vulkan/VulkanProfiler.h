#pragma once

#include <vulkan/vulkan.h>
#include "jaspch.h"

class CommandBuffer;

typedef std::pair<uint64_t, uint64_t> Timestamp;

class VulkanProfiler
{
public:
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

	void init(uint32_t plotDataCount, float updateFreq, TimeUnit timeUnit);
	void cleanup();

	// Render results using ImGui
	void render(float dt);

	void createTimestamps(uint32_t timestampCount);
	void createGraphicsPipelineStats();
	void createComputePipelineStats();
	void addTimestamp(std::string name);

	// Starts a measured timestamp, returns a timestamp object to keep track of the current timestamp
	void startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);
	void startGraphicsPipelineStat(CommandBuffer* commandBuffer);
	void startComputePipelineStat(CommandBuffer* commandBuffer);

	// Ends the selected timestamp using the timestamp object
	void endTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);
	void endGraphicsPipelineStat(CommandBuffer* commandBuffer);
	void endComputePipelineStat(CommandBuffer* commandBuffer);

	// Should be called after a submit has been done to guarantee to retrive correct timestamps (function does sync). Function also resets all timestamps
	void getTimestamps();
	void getPipelineStats();
	void getAllQueries();
	void resetTimestampInterval(CommandBuffer* commandBuffer, uint32_t firstQuery, uint32_t queryCount);
	void resetTimestamp(const std::string& name, CommandBuffer* commandBuffer);
	void resetAllTimestamps(CommandBuffer* commandBuffer);
	void resetPipelineStats(CommandBuffer* commandBuffer);
	void resetAll(CommandBuffer* commandBuffer);

private:
	VulkanProfiler();
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
	std::unordered_map<std::string, Timestamp> results;
	std::unordered_map<std::string, Timestamp> timestamps;
	std::unordered_map<std::string, float> averages;

	std::vector<uint64_t> graphicsPipelineStat;
	std::vector<uint64_t> computePipelineStat;
	std::vector<std::string> graphicsPipelineStatNames;
};