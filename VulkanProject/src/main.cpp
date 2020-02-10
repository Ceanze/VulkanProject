#include "jaspch.h"
#include "Vulkan/Renderer.h"

int main(int argv, char* argc[])
{
	Renderer renderer;
	renderer.init();
	renderer.run();
	renderer.shutdown();

	return 0;
}