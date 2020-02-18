#include "jaspch.h"
#include "Vulkan/Renderer.h"

// TEMP
#include "GLTFTest/GLTFLoader.h"
#include "Threading/ThreadManager.h"

int* data = nullptr;

void preThread(uint32_t threadId)
{
	printf("Started thread %d!\n", threadId);
	data[threadId] = threadId;
}

int main(int argv, char* argc[])
{
	ThreadManager tm;
	uint32_t numThreads = ThreadManager::getMaxNumConcurrentThreads();
	data = new int[numThreads];
	for (uint32_t t = 0; t < numThreads; t++)
		tm.addWork([=]{ preThread(t); });
	tm.join();
	for (uint32_t t = 0; t < numThreads; t++)
		printf("Thread %d -> %d\n", t, data[t]);

	Renderer renderer;
	renderer.init();
	renderer.run();
	renderer.shutdown();

	return 0;
}