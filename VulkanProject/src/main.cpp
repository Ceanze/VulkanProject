#include "jaspch.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Sandbox/SandboxManager.h"
#include "Sandbox/Tests/HeightmapTest.h"
#include "Sandbox/Tests/RenderTest.h"
#include "Sandbox/Tests/ComputeTest/ComputeTest.h"
#include "Sandbox/Tests/GLTFTest.h"
#include "Sandbox/Tests/ThreadingTest.h"

int main(int argv, char* argc[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	SandboxManager sm;
	sm.set(new HeightmapTest());
	sm.init();
	sm.run();
	sm.cleanup();
	
	return 0;
}