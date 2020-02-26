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

void ThreadManager::init(uint32_t numThreads, uint32_t numQueues)
{
	for (uint32_t t = 0; t < numThreads; t++)
		this->threads.push_back(new Thread(numQueues));
}

void ThreadManager::addWork(uint32_t threadIndex, uint32_t queueIndex, std::function<void(void)> work)
{
	this->threads[threadIndex]->addWork(queueIndex, std::move(work));
}

void ThreadManager::wait()
{
	// Wait for all threads to finish.
	for (Thread* thread : this->threads)
		for (uint32_t q = 0; q < thread->getNumQueues(); q++)
			thread->wait(q);
}

bool ThreadManager::isDone(uint32_t queueIndex)
{
	bool ret = true;
	for (Thread* thread : this->threads)
		if (thread->isDone(queueIndex) == false) 
			ret = false;
	return ret;
}

ThreadManager::Thread::Thread(uint32_t numQueues)
{
	this->worker = std::thread(&ThreadManager::Thread::threadLoop, this);
	this->queues.resize(numQueues);
	this->currentQueue = 0;
}

ThreadManager::Thread::~Thread()
{
	if (this->worker.joinable())
	{
		for(uint32_t q = 0; q < this->queues.size(); q++)
			wait(q);
		
		// Lock mutex to be able to change destroying to true.
		std::unique_lock<std::mutex> lock(this->mutex);
		this->destroying = true;
		this->condition.notify_one();
		lock.unlock();

		this->worker.join();
	}
}

void ThreadManager::Thread::addWork(uint32_t queueIndex, std::function<void(void)> work)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	this->queues[queueIndex].push(std::move(work));
	this->condition.notify_one();
}

void ThreadManager::Thread::wait(uint32_t queueIndex)
{
	std::unique_lock<std::mutex> lock(this->mutex);
	this->condition.wait(lock, [this, queueIndex]() { return this->queues[queueIndex].empty(); });
}

bool ThreadManager::Thread::isDone(uint32_t queueIndex)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	return this->queues[queueIndex].empty();
}

uint32_t ThreadManager::Thread::getNumQueues() const
{
	return (uint32_t)this->queues.size();
}

void ThreadManager::Thread::threadLoop()
{
	while (true)
	{
		std::function<void()> work;
		{
			// Lock mutex to be able to access the queue and the 'destroying' variable.
			std::unique_lock<std::mutex> lock(this->mutex);
			this->condition.wait(lock, [this] {
				bool ret = false;
				for (auto q : this->queues)
					if (!q.empty())
						ret = true;
				return ret || destroying;
			});
			if (destroying)
			{
				break;
			}


			for (uint32_t q = 0; q < this->queues.size(); q++)
			{
				auto& queue = this->queues[this->currentQueue];
				if (queue.empty())
					this->currentQueue = (this->currentQueue + 1) % this->queues.size();
				else break;
			}
			work = this->queues[this->currentQueue].front();
		}

		work();

		{
			// Lock mutex to be able to accessthe queue.
			std::lock_guard<std::mutex> lock(this->mutex);
			this->queues[this->currentQueue].pop();
			this->condition.notify_one();
			this->currentQueue = (this->currentQueue + 1) % this->queues.size();
		}
	}
}
