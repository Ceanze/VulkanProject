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
	void init(uint32_t numThreads, uint32_t numQueues);
		
	// Add work to a specific thread.
	void addWork(uint32_t threadIndex, uint32_t queueIndex, std::function<void(void)> work);

	// Wait for all threads to have no work left.
	void wait();

	bool isDone(uint32_t queueIndex);

private:
	struct Thread
	{
		Thread(uint32_t numQueues);
		~Thread();

		void addWork(uint32_t queueIndex, std::function<void(void)> work);

		// Wait for the thread to be done with the queue.
		void wait(uint32_t queueIndex);

		bool isDone(uint32_t queueIndex);

		uint32_t getNumQueues() const;

	private:
		void threadLoop();

		uint32_t currentQueue;
		bool destroying = false;
		std::thread worker;
		std::mutex mutex;
		std::vector<std::queue<std::function<void(void)>>> queues;
		std::condition_variable condition;
	};

	std::vector<Thread*> threads;
};