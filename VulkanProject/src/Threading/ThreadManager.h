#pragma once

#include "jaspch.h"
#include <thread>
#include <functional>

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	// Get the maximum number of concurrent thread the CPU can handle.
	static uint32_t getMaxNumConcurrentThreads();

	// Start a new thread.
	void addWork(std::function<void(void)> function);

	// Wait for all threads to finish.
	void join();

private:
	void clear();

	std::vector<std::thread*> threads;
};