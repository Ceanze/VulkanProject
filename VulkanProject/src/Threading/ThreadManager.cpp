#include "jaspch.h"
#include "ThreadManager.h"

ThreadManager::ThreadManager()
{
}

ThreadManager::~ThreadManager()
{
	clear();
}

uint32_t ThreadManager::getMaxNumConcurrentThreads()
{
	return static_cast<uint32_t>(std::thread::hardware_concurrency());
}

void ThreadManager::addWork(std::function<void(void)> function)
{
	// Start new thread.
	std::thread* thread = new std::thread(function);
	this->threads.push_back(thread);

	thread->get_id();
}

void ThreadManager::join()
{
	// Wait for all threads to finish.
	for (std::thread* thread : this->threads)
		thread->join();

	// Clear thread pool
	clear();
}

void ThreadManager::clear()
{
	for (auto& thread : this->threads)
		delete thread;
}
