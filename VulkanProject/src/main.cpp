#include "jaspch.h"
#include "Vulkan/Renderer.h"
#include "ComputeTest/ComputeTest.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// TEMP
#include "GLTFTest/GLTFLoader.h"

int main(int argv, char* argc[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	/*Renderer renderer;
	renderer.init();
	renderer.run();
	renderer.shutdown();*/

	ComputeTest computeTest;
	computeTest.init();
	computeTest.run();
	computeTest.shutdown(); 

	return 0;
}