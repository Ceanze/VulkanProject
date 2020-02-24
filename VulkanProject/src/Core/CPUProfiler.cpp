#include "jaspch.h"
#include "CPUProfiler.h"

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
		this->startTime = std::chrono::high_resolution_clock::now();
}

void InstrumentationTimer::stop()
{
	if (this->active)
	{
		this->endTime = std::chrono::high_resolution_clock::now();
		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(this->startTime).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(this->endTime).time_since_epoch().count();

		Instrumentation::get().write({ this->name, start, end });
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
	if (this->counter++ > 0) this->file << ",";

	std::string name = data.name;
	std::replace(name.begin(), name.end(), '"', '\'');

	this->file << "{";
	this->file << "\"name\": \"" << name << "\",";
	this->file << "\"cat\": \"function\",";
	this->file << "\"ph\": \"X\",";
	this->file << "\"pid\": 0,";
	this->file << "\"tid\": 0,";
	this->file << "\"ts\": " << data.start << ",";
	this->file << "\"dur\": " << (data.end - data.start);
	this->file << "}";

	this->file.flush();
}

void Instrumentation::endSession()
{
	this->file << "]" << std::endl << "}";
	this->file.close();
}
