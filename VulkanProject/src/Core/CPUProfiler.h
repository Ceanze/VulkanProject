#pragma once

#include <chrono>
#include <string>
#include <fstream>
#include <mutex>

#ifdef JAS_DEBUG
	#define JAS_PROFILER_BEGIN_SESSION(name, fileName) Instrumentation::get().beginSession(name, fileName)
	#define JAS_PROFILER_END_SESSION() Instrumentation::get().endSession()
	#define JAS_PROFILER_SCOPE(name) InstrumentationTimer instrumentationTimer##__LINE__(name)
	#define JAS_PROFILER_FUNCTION() JAS_PROFILER_SCOPE(__FUNCTION__ )

	#define JAS_PROFILER_SAMPLE_BEGIN_SESSION(name, fileName) {if(!Instrumentation::g_runProfilingSample) { Instrumentation::get().beginSession(name, fileName); Instrumentation::g_runProfilingSample = true; }}
	#define JAS_PROFILER_SAMPLE_END_SESSION() {if(Instrumentation::g_runProfilingSample) { Instrumentation::get().endSession(); Instrumentation::g_runProfilingSample = false; } }
	#define JAS_PROFILER_SAMPLE_SCOPE(name) InstrumentationTimer instrumentationTimerRendering##__LINE__(name, Instrumentation::g_runProfilingSample)
	#define JAS_PROFILER_SAMPLE_FUNCTION() JAS_PROFILER_SAMPLE_SCOPE(__FUNCTION__ )

	#define JAS_PROFILER_TOGGLE_SAMPLE(key, frameCount) Instrumentation::get().toggleSample(key, frameCount)
	#define JAS_PROFILER_WRITE_VULKAN_DATA() Instrumentation::get().writeVulkanData()

#else
#define JAS_PROFILER_BEGIN_SESSION(name, fileName)
#define JAS_PROFILER_END_SESSION()
#define JAS_PROFILER_SCOPE(name)
#define JAS_PROFILER_FUNCTION()

#define JAS_PROFILER_SAMPLE_BEGIN_SESSION(name, fileName)
#define JAS_PROFILER_SAMPLE_END_SESSION()
#define JAS_PROFILER_SAMPLE_SCOPE(name)
#define JAS_PROFILER_SAMPLE_FUNCTION()

#define JAS_PROFILER_TOGGLE_SAMPLE(key, frameCount)
#define JAS_PROFILER_WRITE_VULKAN_DATA()
#endif

class InstrumentationTimer
{
public:
	InstrumentationTimer(const std::string& name, bool active = true);
	~InstrumentationTimer();

	void start();
	void stop();

private:
	int64_t startTime;
	std::string name;
	bool active;
};

class Instrumentation
{
public:
	struct ProfileData
	{
		std::string name;
		uint64_t start, end;
		size_t tid = 0;
		size_t pid = 0;
	};
	Instrumentation();
	~Instrumentation();

	static Instrumentation& get();

	void beginSession(const std::string& name, const std::string& filePath = "result.json");

	void write(ProfileData data);

	void setStartTime(uint64_t time);

	void toggleSample(int key, uint32_t frameCount);
	void writeVulkanData();

	void endSession();


	static bool g_runProfilingSample;

private:
	std::mutex mutex;
	std::ofstream file;
	unsigned long long counter;
	uint64_t startTime = 0;
};