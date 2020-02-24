#include "jaspch.h"
#include "Vulkan/Instance.h"
#include "Vulkan/VulkanCommon.h"
#include "Core/Window.h"

#include <GLFW/glfw3.h>

std::vector<const char*> Instance::validationLayers = {
"VK_LAYER_LUNARG_standard_validation"
};

std::vector<const char*> Instance::deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkPhysicalDeviceFeatures Instance::deviceFeatures = {};

Instance::Instance() :
	debugMessenger(VK_NULL_HANDLE), device(VK_NULL_HANDLE), instance(VK_NULL_HANDLE),
	physicalDevice(VK_NULL_HANDLE), surface(VK_NULL_HANDLE)
{

}

Instance::~Instance(){}

Instance& Instance::get()
{
	static Instance i;
	return i;
}

void Instance::init(Window* window)
{
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.logicOp = VK_TRUE;

	this->createInstance();
	this->setupDebugMessenger();
	this->createSurface(window);
	this->pickPhysicalDevice();
	this->createLogicalDevice();

	JAS_INFO("Initilized Instance!");
}

void Instance::cleanup()
{
	vkDestroyDevice(this->device, nullptr);
	if (this->enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	vkDestroyInstance(this->instance, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult Instance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Instance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Instance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo)
{
	debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = debugCallback;
	debugInfo.pUserData = nullptr; // Optional
}

void Instance::setupDebugMessenger()
{
	if (!this->enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	ERROR_CHECK(CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->debugMessenger), "failed to set up debug messenger!");
}

bool Instance::checkValidationLayerSupport() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : this->validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> Instance::getRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (this->enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void Instance::createInstance()
{
	if (this->enableValidationLayers && !checkValidationLayerSupport())
		JAS_ERROR("validation layers requested, but not available!");

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkanProject";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Enable validation layers
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (this->enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	ERROR_CHECK(vkCreateInstance(&createInfo, nullptr, &this->instance), "failed to create instance!");
}

void Instance::createSurface(Window* window)
{
	ERROR_CHECK(glfwCreateWindowSurface(this->instance, window->getNativeWindow(), nullptr, &this->surface), "Failed to create surface!");
}

void Instance::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) 
		JAS_ERROR("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	int score = 0;
	for (const auto& device : devices) {
		int newScore = rateDeviceSutiable(device);
		if (newScore > score) {
			physicalDevice = device;
			score = newScore;
		}
	}

	if (score == 0) {
		throw std::runtime_error("There is no physical device suitable!");
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void Instance::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice, this->surface);
	this->graphicsQueue.queueIndex = indices.graphicsFamily.value();
	this->presentQueue.queueIndex = indices.presentFamily.value();
	this->transferQueue.queueIndex = findQueueIndex(VK_QUEUE_TRANSFER_BIT, this->physicalDevice);
	this->computeQueue.queueIndex = findQueueIndex(VK_QUEUE_COMPUTE_BIT, this->physicalDevice);
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { this->graphicsQueue.queueIndex, this->presentQueue.queueIndex,
		this->transferQueue.queueIndex, this->computeQueue.queueIndex};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &this->deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	createInfo.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
	createInfo.ppEnabledLayerNames = this->validationLayers.data();

	// Create the logical device
	ERROR_CHECK(vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS, "failed to create logical device!");

	// Get all desired queues
	vkGetDeviceQueue(this->device, this->graphicsQueue.queueIndex, 0, &this->graphicsQueue.queue);
	vkGetDeviceQueue(this->device, this->presentQueue.queueIndex, 0, &this->presentQueue.queue);
	vkGetDeviceQueue(this->device, this->transferQueue.queueIndex, 0, &this->transferQueue.queue);
	vkGetDeviceQueue(this->device, this->computeQueue.queueIndex, 0, &this->computeQueue.queue);
}

bool Instance::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices index = findQueueFamilies(device, this->surface);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, this->surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	if (!index.isComplete() && !extensionsSupported && !swapChainAdequate && !deviceFeatures.samplerAnisotropy) {
		return false;
	}

	return true;
}

bool Instance::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

int Instance::rateDeviceSutiable(VkPhysicalDevice device)
{
	if (!isDeviceSuitable(device))
		return 0;
	
	int score = 0;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	JAS_TRACE("Device type[{}]: Device name: {} Score: {}", deviceProperties.deviceID, deviceProperties.deviceName, score);

	return score;
}
