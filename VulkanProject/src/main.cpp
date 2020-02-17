#include "jaspch.h"
#include "Vulkan/Renderer.h"

// TEMP
#include "GLTFTest/GLTFLoader.h"

int main(int argv, char* argc[])
{
	//Renderer renderer;
	//renderer.init();
	//renderer.run();
	//renderer.shutdown();

	GLTFLoader gltfLoader;
	gltfLoader.init();
	gltfLoader.run();
	gltfLoader.cleanup();

	return 0;
}