#include "jaspch.h"
#include "SandboxManager.h"

#include "VKSandboxBase.h"
#include "Vulkan/Instance.h"
#include <GLFW/glfw3.h>

SandboxManager::SandboxManager() : running(true)
{
}

SandboxManager::~SandboxManager()
{
}

void SandboxManager::add(VKSandboxBase* sandbox)
{
	this->sandboxes.push_back(sandbox);
}

void SandboxManager::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

	for (auto& sandbox : this->sandboxes)
	{
		sandbox->setWindow(&this->window);
		sandbox->setSwapChain(&this->swapChain);
		sandbox->selfInit();
	}
}

void SandboxManager::run()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto prevTime = currentTime;
	float dt = 0;
	unsigned frames = 0;
	double elapsedTime = 0;

	while (this->running && this->window.isOpen())
	{
		glfwPollEvents();

		if (glfwGetKey(this->window.getNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			this->running = false;

		for (auto& sandbox : this->sandboxes)
			sandbox->selfLoop(dt);

		currentTime = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float>(currentTime - prevTime).count();
		elapsedTime += dt;
		frames++;
		prevTime = currentTime;

		if (elapsedTime >= 1.0) {
			this->window.setTitle("FPS: " + std::to_string(frames) + " [Delta time: " + std::to_string((elapsedTime / frames) * 1000.f) + " ms]");
			elapsedTime = 0;
			frames = 0;
		}
	}
}

void SandboxManager::cleanup()
{
	for (auto& sandbox : this->sandboxes)
	{
		sandbox->selfCleanup();
		delete sandbox;
	}
	this->sandboxes.clear();

	this->swapChain.cleanup();
	Instance::get().cleanup();
	this->window.cleanup();
}
