#include "jaspch.h"
#include "Renderer.h"

#include <GLFW/glfw3.h>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");

	JAS_INFO("Created renderer!");
}

void Renderer::run()
{
	while (running && this->window.isOpen())
	{
		glfwPollEvents();
	}
}

void Renderer::shutdown()
{
	this->window.cleanup();
}
