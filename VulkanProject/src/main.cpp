#include "jaspch.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Sandbox/SandboxManager.h"
#include "Sandbox/ProjectFinal.h"
#include "Sandbox/ProjectFinalNaive.h"

#include "Core/CPUProfiler.h"

	/*
		---------------Controls---------------
			WASD:	Move camera
			Mouse:	Look around
			R:		Start frame profiling
			C:		Toggle camera
			F:		Toggle gravity
			ESC:	Exit

		----------Implementation--------------
			Change parameters in the Config.h

			Change MULTI_THREADED to false
			to run the single threaded
			implementation.

		------------Information--------------
			- Program can be closed with ESCAPE
	*/

#include "Config.h"

int main(int argv, char* argc[])
{
#ifdef JAS_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif 

	JAS_PROFILER_BEGIN_SESSION("Profiling", PROFILER_JSON_FILE_NAME);

	SandboxManager sm;
#if MULTI_THREADED
	sm.set(new ProjectFinal());
#else
	sm.set(new ProjectFinalNaive());
#endif
	sm.init();
	sm.run();
	sm.cleanup();

	JAS_PROFILER_END_SESSION();
	
	return 0;
}