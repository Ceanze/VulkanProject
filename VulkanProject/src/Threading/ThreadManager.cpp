#include "jaspch.h"
#include "ThreadManager.h"
#include "Core/CPUProfiler.h"

std::vector<ThreadManager::Thread*> ThreadManager::threads;

void ThreadManager::init(uint32_t numThreads)
{
	for (uint32_t t = 0; t < numThreads; t++)
		threads.push_back(new Thread());
}

void ThreadManager::cleanup()
{
	// Delete all threads
	for (Thread* thread : threads)
		delete thread;

	// Clear thread pool
	threads.clear();
}

void ThreadManager::addWork(uint32_t threadIndex, std::function<void(void)> work)
{
	threads[threadIndex]->addWork(0, std::move(work));
}

uint32_t ThreadManager::addWorkTrace(uint32_t threadIndex, std::function<void(void)> work)
{
	static uint32_t currentId = 1; // Max works in parallel = UINT32_MAX - 1
	if (currentId == 0)
		currentId++;

	threads[threadIndex]->addWork(currentId, std::move(work));
	return currentId++;
}

void ThreadManager::wait()
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	// Wait for all threads to finish.
	bool done = false;
	while (!done) {
		done = true;
		for (Thread* thread : threads)
			done &= thread->isQueueEmpty();
	}
}

bool ThreadManager::isQueueEmpty()
{
	bool ret = true;
	for (Thread* thread : threads)
		if (thread->isQueueEmpty() == false)
			ret = false;
	return ret;
}

bool ThreadManager::isWorkFinished(uint32_t id)
{
	bool ret = false;
	for (size_t t = 0; t < threads.size(); t++)
	{
		ret |= threads[t]->isWorkFinished(id);
	}
	return ret;
}

bool ThreadManager::isWorkFinished(uint32_t id, uint32_t threadID)
{
	return threads[threadID]->isWorkFinished(id);
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

void ThreadManager::Thread::addWork(uint32_t id, std::function<void(void)> work)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	this->queue.push({ id, std::move(work) });
	this->condition.notify_one();
}

void ThreadManager::Thread::wait()
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	std::unique_lock<std::mutex> lock(this->mutex);
	this->condition.wait(lock, [this]() { return this->queue.empty(); });
}

bool ThreadManager::Thread::isQueueEmpty()
{
	/*std::lock_guard<std::mutex> lock(this->mutex);*/
	return this->queue.empty();
}

bool ThreadManager::Thread::isWorkFinished(uint32_t id)
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
	return (uint32_t)this->queue.size();
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
				if (!this->queue.empty())
					ret = true;
				return ret || destroying;
			});
			if (destroying)
			{
				break;
			}

			work = this->queue.front().second;
		}
		
		work();
	
		{
			JAS_PROFILER_SAMPLE_SCOPE("TF");
			// Lock mutex to be able to access the queue.
			std::lock_guard<std::mutex> lock(this->mutex);
			auto& currentQueue = this->queue;
			auto workId = currentQueue.front().first;
			if (workId != 0)
				this->worksDone.push_back(workId);
			currentQueue.pop();
			this->condition.notify_one();
		}

	}
}
