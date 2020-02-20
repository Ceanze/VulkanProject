#pragma once

// Code from: https://github.com/SaschaWillems/Vulkan/blob/master/base/threadpool.hpp

#include "jaspch.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadManager
{
public:
	

public:
	ThreadManager();
	~ThreadManager();

	// Get the maximum number of concurrent thread the CPU can handle.
	static uint32_t getMaxNumConcurrentThreads();

	// Start new threads.
	void init(uint32_t numThreads);
		
	// Add work to a specific thread.
	void addWork(uint32_t threadIndex, std::function<void(void)> work);

	// Wait for all threads to have no work left.
	void wait();

private:
	struct Thread
	{
		Thread();
		~Thread();

		void addWork(std::function<void(void)> work);

		// Wait for the thread to be done with the queue.
		void wait();

	private:
		void threadLoop();

		bool destroying = false;
		std::thread worker;
		std::mutex mutex;
		std::queue<std::function<void(void)>> queue;
		std::condition_variable condition;
	};

	std::vector<Thread*> threads;
};