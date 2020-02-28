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
	this->threads[threadIndex]->addWork(0, queueIndex, std::move(work));
}

ThreadManager::WorkID ThreadManager::addWorkTrace(uint32_t threadIndex, uint32_t queueIndex, std::function<void(void)> work)
{
	static WorkID currentId = 1; // Max works in parallel = UINT32_MAX - 1
	if (currentId == 0)
		currentId++;

	this->threads[threadIndex]->addWork(currentId, queueIndex, std::move(work));
	return currentId++;
}

void ThreadManager::wait()
{
	// Wait for all threads to finish.
	for (Thread* thread : this->threads)
		for (uint32_t q = 0; q < thread->getNumQueues(); q++)
			thread->wait(q);
}

bool ThreadManager::isQueueEmpty(uint32_t queueIndex)
{
	bool ret = true;
	for (Thread* thread : this->threads)
		if (thread->isQueueEmpty(queueIndex) == false)
			ret = false;
	return ret;
}

bool ThreadManager::isWorkFinished(WorkID id)
{
	bool ret = false;
	for (size_t t = 0; t < this->threads.size(); t++)
	{
		ret |= this->threads[t]->isWorkFinished(id);
	}
	return ret;
}

bool ThreadManager::isWorkFinished(WorkID id, uint32_t threadID)
{
	return this->threads[threadID]->isWorkFinished(id);
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

void ThreadManager::Thread::addWork(ThreadManager::WorkID id, uint32_t queueIndex, std::function<void(void)> work)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	this->queues[queueIndex].push({ id, std::move(work) });
	this->condition.notify_one();
}

void ThreadManager::Thread::wait(uint32_t queueIndex)
{
	std::unique_lock<std::mutex> lock(this->mutex);
	this->condition.wait(lock, [this, queueIndex]() { return this->queues[queueIndex].empty(); });
}

bool ThreadManager::Thread::isQueueEmpty(uint32_t queueIndex)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	return this->queues[queueIndex].empty();
}

bool ThreadManager::Thread::isWorkFinished(WorkID id)
{
	std::lock_guard<std::mutex> lck(this->mutex);
	auto it = std::find(this->worksDone.begin(), this->worksDone.end(), id);

	if (it == this->worksDone.end())
		return false;
	else {
		this->worksDone.erase(it);
		return true;
	}
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
			work = this->queues[this->currentQueue].front().second;
		}

		work();

		{
			// Lock mutex to be able to accessthe queue.
			std::lock_guard<std::mutex> lock(this->mutex);
			auto& currentQueue = this->queues[this->currentQueue];
			auto workId = currentQueue.front().first;
			if (workId != 0)
				this->worksDone.push_back(workId);
			currentQueue.pop();
			this->condition.notify_one();
			this->currentQueue = (this->currentQueue + 1) % this->queues.size();
		}
	}
}
