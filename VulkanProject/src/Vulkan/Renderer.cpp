#include "jaspch.h"
#include "Renderer.h"
#include "Vulkan/Instance.h"

#include <GLFW/glfw3.h>
#include "Pipeline/Renderpass.h"

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

	this->shader.addStage(Shader::VERTEX, "testVertex.spv");
	this->shader.addStage(Shader::FRAGMENT, "testFragment.spv");
	this->shader.init();

	RenderPass renderPass;
	renderPass.addDefaultColorAttachment(VK_FORMAT_B8G8R8A8_SRGB);
	renderPass.addDefaultDepthAttachment();
	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
	subpassInfo.depthStencilAttachmentIndex = { 1 };
	renderPass.addSubpass(subpassInfo);
	renderPass.addDefaultSubpassDependency();
	renderPass.init();

	renderPass.cleanup();

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
	this->shader.cleanup();
	this->swapChain.cleanup();
	Instance::get().cleanup();
	this->window.cleanup();
}
