#include "jaspch.h"
#include "ThreadManager.h"

ThreadManager::ThreadManager()
{
}

ThreadManager::~ThreadManager()
{
	// Delete all threads
	for (Thread* thread : this->threads)
		delete thread;
	
	// Clear thread pool
	this->threads.clear();
}

uint32_t ThreadManager::getMaxNumConcurrentThreads()
{
	return static_cast<uint32_t>(std::thread::hardware_concurrency());
}

void ThreadManager::init(uint32_t numThreads)
{
	for (uint32_t t = 0; t < numThreads; t++)
		this->threads.push_back(new Thread());
}

void ThreadManager::addWork(uint32_t threadIndex, std::function<void(void)> work)
{
	this->threads[threadIndex]->addWork(std::move(work));
}

void ThreadManager::wait()
{
	// Wait for all threads to finish.
	for (Thread* thread : this->threads)
		thread->wait();
}

ThreadManager::Thread::Thread()
{
	this->worker = std::thread(&ThreadManager::Thread::threadLoop, this);
}

ThreadManager::Thread::~Thread()
{
	if (this->worker.joinable())
	{
		wait();
		
		// Lock mutex to be able to change destroying to true.
		std::unique_lock<std::mutex> lock(this->mutex);
		this->destroying = true;
		this->condition.notify_one();
		lock.unlock();

		this->worker.join();
	}
}

void ThreadManager::Thread::addWork(std::function<void(void)> work)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	this->queue.push(std::move(work));
	this->condition.notify_one();
}

void ThreadManager::Thread::wait()
{
	std::unique_lock<std::mutex> lock(this->mutex);
	this->condition.wait(lock, [this]() { return this->queue.empty(); });
}

void ThreadManager::Thread::threadLoop()
{
	while (true)
	{
		std::function<void()> work;
		{
			// Lock mutex to be able to accessthe queue and the 'destroying' variable.
			std::unique_lock<std::mutex> lock(this->mutex);
			this->condition.wait(lock, [this] { return !this->queue.empty() || destroying; });
			if (destroying)
			{
				break;
			}
			work = this->queue.front();
		}

		work();

		{
			// Lock mutex to be able to accessthe queue.
			std::lock_guard<std::mutex> lock(this->mutex);
			this->queue.pop();
			this->condition.notify_one();
		}
	}
}
