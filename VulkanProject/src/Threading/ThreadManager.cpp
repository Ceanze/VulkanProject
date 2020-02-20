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
	clear();
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

void ThreadManager::clear()
{
	for (auto& thread : this->threads)
		delete thread;
	this->threads.clear();
}

ThreadManager::Thread::Thread()
{
	static uint32_t uid = 0;
	this->id = uid++;
	this->worker = std::thread(&ThreadManager::Thread::threadLoop, this);
	JAS_INFO("Thread {0} created!", this->id);
}

ThreadManager::Thread::~Thread()
{
	if (this->worker.joinable())
	{
		wait();
		
		// Lock mutex to be able to change destroying to true.
		this->mutex.lock();
		this->destroying = true;
		this->condition.notify_one();
		this->mutex.unlock();

		this->worker.join();
		JAS_INFO("Thread {0} destroyed!", this->id);
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
