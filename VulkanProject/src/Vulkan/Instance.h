#pragma once

#include <vulkan/vulkan.h>
#include "jaspch.h"

class Window;

struct QueueVK {
	QueueVK() : queue(VK_NULL_HANDLE), queueIndex(0) {};

	VkQueue queue;
	uint32_t queueIndex;
};

class Instance
{
public:
	~Instance();

	static Instance& get();

	void init(Window* window);
	void cleanup();

	VkSurfaceKHR getSurface() const { return this->surface; }
	VkDevice getDevice() const { return this->device; }
	VkPhysicalDevice getPhysicalDevice() const { return this->physicalDevice; }

	QueueVK getGraphicsQueue() const { return this->graphicsQueue; }
	QueueVK getPresentQueue() const { return this->presentQueue; }
	QueueVK getTransferQueue() const { return this->transferQueue; }
	QueueVK getComputeQueue() const { return this->computeQueue; }

private:
	Instance();
	Instance(Instance& other) = delete;

	bool checkValidationLayerSupport() const;
	std::vector<const char*> getRequiredExtensions() const;

	static std::vector<const char*> validationLayers;
	static std::vector<const char*> deviceExtensions;
	static VkPhysicalDeviceFeatures deviceFeatures;

	void createInstance();
	void createSurface(Window* window);
	void pickPhysicalDevice();
	void createLogicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice device);
	int rateDeviceSutiable(VkPhysicalDevice device);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkInstance instance;

	QueueVK graphicsQueue;
	QueueVK presentQueue;
	QueueVK transferQueue;
	QueueVK computeQueue;

	#ifdef JAS_RELEASE
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif
	
	/*
		Debug Callback
	*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo);
	void setupDebugMessenger();
};
