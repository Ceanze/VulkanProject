#include "jaspch.h"
#include "Vulkan/Renderer.h"

// TEMP
#include "Threading/ThreadingTest.h"

int main(int argv, char* argc[])
{
	ThreadingTest tt;
	tt.init();
	tt.run();
	tt.shutdown();

	/*Renderer renderer;
	renderer.init();
	renderer.run();
	renderer.shutdown();
	*/
	return 0;
}