#include "jaspch.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Sandbox/SandboxManager.h"
#include "Sandbox/Tests/HeightmapTest.h"
#include "Sandbox/Tests/RenderTest.h"
#include "Sandbox/Tests/ComputeTest/ComputeTest.h"
#include "Sandbox/Tests/GLTFTest.h"
//#include "Sandbox/Tests/ThreadingTest.h"
#include "Sandbox/Tests/NoThreadingTest.h"
#include "Sandbox/Tests/TransferTest.h"
#include "Sandbox/Tests/CubemapTest.h"
#include "Sandbox/Tests/ComputeTransferTest.h"
#include "Sandbox/ProjectFinal.h"

#include "Core/CPUProfiler.h"

int main(int argv, char* argc[])
{
	JAS_PROFILER_BEGIN_SESSION("Profiling", "results.json");

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	SandboxManager sm;
	sm.set(new ProjectFinal());
	sm.init();
	sm.run();
	sm.cleanup();

	JAS_PROFILER_END_SESSION();
	
	return 0;
}