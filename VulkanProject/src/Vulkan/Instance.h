#pragma once

#include <vulkan/vulkan.h>
#include "jaspch.h"

class Window;

class Instance
{
public:
	~Instance();

	static Instance& get();

	void init(Window* window);
	void cleanup();

	VkSurfaceKHR getSurface();
	VkDevice getDevice();
	VkPhysicalDevice getPhysicalDevice();

	VkQueue getGraphicsQueue() const;
	VkQueue getPresentQueue() const;

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

	// Temporary
	VkQueue graphicsQueue;
	VkQueue presentQueue;

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
