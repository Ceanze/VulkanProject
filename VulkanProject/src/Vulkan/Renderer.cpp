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

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

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
	this->swapChain.cleanup();
	Instance::get().cleanup();
	this->window.cleanup();
}
