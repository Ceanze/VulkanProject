#pragma once

#include <vulkan/vulkan.h>
#include "jaspch.h"

class CommandBuffer;

typedef std::pair<uint64_t, uint64_t> Timestamp;

class VulkanProfiler
{
public:
	static VulkanProfiler& get();
	VulkanProfiler(VulkanProfiler& other) = delete;
	~VulkanProfiler();

	void init(uint32_t plotDataCount, float updateFreq);
	void cleanup();

	// Render results using ImGui
	void render(float dt);

	void createTimestamps(uint32_t timestampCount);
	void createPipelineStats();
	void addTimestamp(std::string name);

	// Starts a measured timestamp, returns a timestamp object to keep track of the current timestamp
	void startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);
	void startPipelineStat(CommandBuffer* commandBuffer);

	// Ends the selected timestamp using the timestamp object
	void endTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);
	void endPipelineStat(CommandBuffer* commandBuffer);

	// Should be called after a submit has been done to guarantee to retrive correct timestamps (function does sync). Function also resets all timestamps
	void getTimestamps();
	void getPipelineStat();
	void getAllQueries();
	void resetTimestampInterval(CommandBuffer* commandBuffer, uint32_t firstQuery, uint32_t queryCount);
	void resetTimestamp(const std::string& name, CommandBuffer* commandBuffer);
	void resetAllTimestamps(CommandBuffer* commandBuffer);
	void resetPipelineStat(CommandBuffer* commandBuffer);
	void resetAll(CommandBuffer* commandBuffer);

private:
	VulkanProfiler();

	VkQueryPool timestampQueryPool;
	VkQueryPool pipelineStatPool;
	uint32_t timestampCount;
	//uint32_t pipelineStatCount;
	size_t freeIndex;
	size_t plotDataCount;
	float timeSinceUpdate;
	float updateFreq;
	std::unordered_map<std::string, std::vector<float>> plotResults;
	std::unordered_map<std::string, Timestamp> results;
	std::unordered_map<std::string, Timestamp> timestamps;
	std::unordered_map<std::string, float> averages;

	std::vector<uint64_t> pipelineStat;
	std::vector<std::string> pipelineStatNames;
};