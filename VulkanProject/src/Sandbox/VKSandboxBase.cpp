#include "jaspch.h"
#include "VKSandboxBase.h"

VKSandboxBase::VKSandboxBase() : window(nullptr), swapChain(nullptr), framebuffersInitialized(false)
{
}

VKSandboxBase::~VKSandboxBase()
{
}

void VKSandboxBase::selfInit()
{
	this->framebuffers.resize(getSwapChain()->getNumImages());
	this->frame.init(this->swapChain);

	init();

	JAS_ASSERT(this->framebuffersInitialized == true, "Framebuffers must be initialized by the function initFramebuffers()!");
}

void VKSandboxBase::selfLoop(float dt)
{
	loop(dt);
}

void VKSandboxBase::selfCleanup()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	cleanup();

	this->frame.cleanup();

	for (auto& framebuffer : this->framebuffers)
		framebuffer.cleanup();
}

void VKSandboxBase::initFramebuffers(RenderPass* renderPass, VkImageView depthAttachment)
{
	this->framebuffersInitialized = true;
	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(getSwapChain()->getImageViews()[i]);  // Add color attachment
		if(depthAttachment != VK_NULL_HANDLE)
			imageViews.push_back(depthAttachment); // Add depth image view
		getFramebuffers()[i].init(getSwapChain()->getNumImages(), renderPass, imageViews, getSwapChain()->getExtent());
	}
}

Window* VKSandboxBase::getWindow()
{
	return this->window;
}

SwapChain* VKSandboxBase::getSwapChain()
{
	return this->swapChain;
}

std::vector<Framebuffer>& VKSandboxBase::getFramebuffers()
{
	return this->framebuffers;
}

Frame* VKSandboxBase::getFrame()
{
	return &this->frame;
}

void VKSandboxBase::setWindow(Window* window)
{
	this->window = window;
}

void VKSandboxBase::setSwapChain(SwapChain* swapChain)
{
	this->swapChain = swapChain;
}
