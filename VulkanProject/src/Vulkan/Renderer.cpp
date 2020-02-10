#include "jaspch.h"
#include "Renderer.h"
#include "Vulkan/Instance.h"

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

	Instance& i = Instance::get();
	i.init(&this->window);
	JAS_INFO("Initilized Instance!");

	JAS_INFO("Created Renderer!");
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
