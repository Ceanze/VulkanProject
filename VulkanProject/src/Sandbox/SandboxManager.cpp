#include "jaspch.h"
#include "SandboxManager.h"

#include "VKSandboxBase.h"
#include "Vulkan/Instance.h"
#include <GLFW/glfw3.h>

SandboxManager::SandboxManager() : running(true), currentSandbox(0)
{
}

SandboxManager::~SandboxManager()
{
}

void SandboxManager::set(VKSandboxBase* sandbox)
{
	this->sandboxes.push_back(sandbox);
}

void SandboxManager::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());
	this->frame.init(&this->window, &this->swapChain);


	this->sandboxes[this->currentSandbox]->setWindow(&this->window);
	this->sandboxes[this->currentSandbox]->setSwapChain(&this->swapChain);
	this->sandboxes[this->currentSandbox]->setFrame(&this->frame);
	this->sandboxes[this->currentSandbox]->selfInit();
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

		this->sandboxes[this->currentSandbox]->selfLoop(dt);

		{
			// Check if you swap sandbox.
			if (glfwGetKey(this->window.getNativeWindow(), GLFW_KEY_PAGE_UP) == GLFW_PRESS)
			{
			}
			else if (glfwGetKey(this->window.getNativeWindow(), GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
			{

			}
		}

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
	this->sandboxes[this->currentSandbox]->selfCleanup();
	for(auto sandbox : this->sandboxes)
		delete sandbox;
	this->sandboxes.clear();

	this->frame.cleanup();
	this->swapChain.cleanup();
	Instance::get().cleanup();
	this->window.cleanup();
}
