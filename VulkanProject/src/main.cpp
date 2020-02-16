#include "jaspch.h"
#include "Vulkan/Renderer.h"

// TEMP
#include "GLTFTest/GLTFTest.h"

int main(int argv, char* argc[])
{
	//Renderer renderer;
	//renderer.init();
	//renderer.run();
	//renderer.shutdown();

	GLTFTest gltfTest;
	gltfTest.init();
	gltfTest.run();
	gltfTest.cleanup();

	return 0;
}