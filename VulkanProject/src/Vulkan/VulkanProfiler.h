#pragma once

#include <vulkan/vulkan.h>
#include "jaspch.h"

/*
	Global singleton class used for profiling vulkan commands (vkCmd) using timestamps

	TODO:
		- Add working timestamps
		- Add queries
		- Add ImGui rendering of results
		- Add a safe way to get freeQueryIndex
		- Add a way to differentiate the different timestamp pairs (name, tag, ...)
		
		- Might have reset in addTimestamp
*/

class CommandBuffer;

typedef std::pair<uint64_t, uint64_t> Timestamp;

class VulkanProfiler
{
public:
	static VulkanProfiler& get();
	VulkanProfiler(VulkanProfiler& other) = delete;
	~VulkanProfiler();

	void init(uint32_t timestampCount, uint32_t plotDataCount, float updateFreq);
	void cleanup();

	// Render results using ImGui
	void render(float dt);

	void addTimestamp(std::string name);

	// Starts a measured timestamp, returns a timestamp object to keep track of the current timestamp
	void startTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);

	// Ends the selected timestamp using the timestamp object
	void endTimestamp(std::string name, CommandBuffer* commandBuffer, VkPipelineStageFlagBits pipelineStage);

	// Should be called after a submit has been done to guarantee to retrive correct timestamps (function does sync). Function also resets all timestamps
	void getTimestamps();
	void resetTimestampInterval(CommandBuffer* commandBuffer, uint32_t firstQuery, uint32_t queryCount);
	void resetTimestamp(const std::string& name, CommandBuffer* commandBuffer);
	void resetAll(CommandBuffer* commandBuffer);

private:
	VulkanProfiler();

	VkQueryPool queryPool;
	uint32_t queryCount;
	size_t freeIndex;
	size_t plotDataCount;
	float timeSinceUpdate;
	float updateFreq;
	std::unordered_map<std::string, std::vector<float>> plotResults;
	std::unordered_map<std::string, Timestamp> results;
	std::unordered_map<std::string, Timestamp> timestamps;
	std::unordered_map<std::string, float> averages;
};