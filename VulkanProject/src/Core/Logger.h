#pragma once
#pragma 

#pragma warning(push)
#pragma warning( disable : 26439 26451 26495 26439 26451 26495 26812 6385)
#include "spdlog/spdlog.h"
#pragma warning(pop)


#pragma warning(push)
#pragma warning(disable:4251)

class Logger
{
public:
	static void init();

	inline static std::shared_ptr<spdlog::logger> getCoreLogger() { return coreLogger; }
	inline static std::shared_ptr<spdlog::logger> getClientLogger() { return clientLogger; }

private:
	static std::shared_ptr<spdlog::logger> coreLogger;
	static std::shared_ptr<spdlog::logger> clientLogger;
};

// Poly logger macros
#define JAS_TRACE(...) Logger::getCoreLogger()->trace(__VA_ARGS__)
#define JAS_INFO(...) Logger::getCoreLogger()->info(__VA_ARGS__)
#define JAS_WARN(...) Logger::getCoreLogger()->warn(__VA_ARGS__)
#define JAS_ERROR(...) Logger::getCoreLogger()->error(__VA_ARGS__)
#define JAS_FATAL(...) Logger::getCoreLogger()->critical(__VA_ARGS__)

#pragma warning(pop)