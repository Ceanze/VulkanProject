#include "jaspch.h"
#include "Logger.h"

#pragma warning(push)
#pragma warning( disable : 26439 26451 26495 26439 26451 26495 )
#include "spdlog/sinks/stdout_color_sinks.h"
#pragma warning(pop)


std::shared_ptr<spdlog::logger> Logger::coreLogger;
std::shared_ptr<spdlog::logger> Logger::clientLogger;


void Logger::init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$"); // Sets color and format of "[TIME] LOGGER: MSG"

	coreLogger = spdlog::stderr_color_mt("CORE");
	coreLogger->set_level(spdlog::level::trace);
}