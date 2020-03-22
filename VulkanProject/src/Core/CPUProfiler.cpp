#include "jaspch.h"
#include "CPUProfiler.h"
#include "Core/Input.h"
#include "Vulkan/VulkanProfiler.h"

// Code modifed from: https://github.com/TheCherno/Hazel

/*
	InstrumentationTimer class
*/

InstrumentationTimer::InstrumentationTimer(const std::string& name, bool active) : name(name), active(active)
{
	start();
}

InstrumentationTimer::~InstrumentationTimer()
{
	stop();
}

void InstrumentationTimer::start()
{
	if (this->active)
		this->startTime = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
}

void InstrumentationTimer::stop()
{
	if (this->active)
	{
		long long start = this->startTime;
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();

		size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
		Instrumentation::get().write({ this->name, (uint64_t)start, (uint64_t)end, tid });
	}
}

/*
	Instrumentation class
*/

bool Instrumentation::g_runProfilingSample = false;

Instrumentation::Instrumentation() : counter(0)
{
}

Instrumentation::~Instrumentation()
{
}

Instrumentation& Instrumentation::get()
{
	static Instrumentation instrumentation;
	return instrumentation;
}

void Instrumentation::beginSession(const std::string& name, const std::string& filePath)
{
	this->counter = 0;
	this->file.open(filePath);
	this->file << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
	this->file.flush();
}

void Instrumentation::write(ProfileData data)
{
	if (data.pid == 0) {
		data.start -= this->startTime;
		data.end -= this->startTime;
	}

	std::lock_guard<std::mutex> lock(this->mutex);
	if (this->counter++ > 0) this->file << ",";

	std::string name = data.name;
	std::replace(name.begin(), name.end(), '"', '\'');

	this->file << "\n{";
	this->file << "\"name\": \"" << name << "\",";
	this->file << "\"cat\": \"function\",";
	this->file << "\"ph\": \"X\",";
	this->file << "\"pid\": " << data.pid << ",";
	this->file << "\"tid\": " << data.tid << ",";
	this->file << "\"ts\": " << data.start << ",";
	this->file << "\"dur\": " << (data.end - data.start);
	this->file << "}";

	this->file.flush();
}

void Instrumentation::setStartTime(uint64_t time)
{
	this->startTime = time;
}

void Instrumentation::toggleSample(int key, uint32_t frameCount)
{
	static uint32_t frameCounter = 0;
	if (Input::get().getKeyState(key) == Input::KeyState::FIRST_RELEASED)
	{
		frameCounter = 0;
		Instrumentation::g_runProfilingSample = true;
		JAS_INFO("Start Profiling");
	}

	if (Instrumentation::g_runProfilingSample)
	{
		frameCounter++;
		if (frameCounter > frameCount)
		{
			Instrumentation::g_runProfilingSample = false;
			writeVulkanData();
			JAS_INFO("End Profiling");
		}
	}
}

void Instrumentation::toggleSample(CommandPool* pool, int key, uint32_t frameCount)
{
	static uint32_t frameCounter = 0;
	if (Input::get().getKeyState(key) == Input::KeyState::FIRST_RELEASED)
	{
		frameCounter = 0;
		VulkanProfiler::get().setupTimers(pool);
		Instrumentation::g_runProfilingSample = true;
		JAS_INFO("Start Profiling");
	}

	if (Instrumentation::g_runProfilingSample)
	{
		frameCounter++;
		if (frameCounter > frameCount)
		{
			Instrumentation::g_runProfilingSample = false;
			writeVulkanData();
			JAS_INFO("End Profiling");
		}
	}
}

void Instrumentation::writeVulkanData()
{
	auto& results = VulkanProfiler::get().getResults();

	for (auto& res : results) {
		for (uint32_t i = 0; i < res.second.size(); i++) {
			write({ res.first + std::to_string(res.second[i].id), res.second[i].start, res.second[i].end, 0, 1 });
		}
	}
}

void Instrumentation::endSession()
{
	this->file << "]" << std::endl << "}";
	this->file.close();
}
