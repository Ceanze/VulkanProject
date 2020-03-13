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
	// Start new threads.
	static void init(uint32_t numThreads);
	static void cleanup();
		
	// Add work to a specific thread.
	static void addWork(uint32_t threadIndex, std::function<void(void)> work);
	static uint32_t addWorkTrace(uint32_t threadIndex, std::function<void(void)> work);

	// Wait for all threads to have no work left.
	static void wait();

	static uint32_t threadCount() { return threads.size(); };
	static bool isQueueEmpty();
	static bool isWorkFinished(uint32_t id);
	static bool isWorkFinished(uint32_t id, uint32_t threadID);

private:
	ThreadManager() = delete;
	~ThreadManager() = default;
	struct Thread
	{
		Thread();
		~Thread();

		void addWork(uint32_t id, std::function<void(void)> work);

		// Wait for the thread to be done with the queue.
		void wait();

		bool isQueueEmpty();
		bool isWorkFinished(uint32_t id);

		uint32_t getNumQueues() const;

	private:
		void threadLoop();

		bool destroying = false;
		std::thread worker;
		std::mutex mutex;
		std::vector<uint32_t> worksDone;
		std::queue<std::pair<uint32_t, std::function<void(void)>>> queue;
		std::condition_variable condition;
	};

	static std::vector<Thread*> threads;
};