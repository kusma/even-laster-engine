#include "vkinstance.h"

#include "core/core.h"

#include <assert.h>
#include <stdio.h>
#include <cstring>

#include "vkhelpers.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace vkHelpers;

using std::vector;
using std::function;
using std::runtime_error;

VkInstance vkInstance::instance;
VkDevice vkInstance::device;
VkPhysicalDevice vkInstance::physicalDevice;
VkPhysicalDeviceFeatures vkInstance::enabledFeatures = { 0 };
VkPhysicalDeviceProperties vkInstance::deviceProperties;
VkPhysicalDeviceMemoryProperties vkInstance::deviceMemoryProperties;
uint32_t vkInstance::graphicsQueueIndex = UINT32_MAX;
VkQueue vkInstance::graphicsQueue;
VkCommandPool vkInstance::setupCommandPool;
VkDebugReportCallbackEXT vkInstance::debugReportCallback;

#ifndef NDEBUG
static VkBool32 messageCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	size_t location,
	int32_t msgCode,
	const char *pLayerPrefix,
	const char *pMsg,
	void *pUserData)
{
	size_t message_len = strlen(pMsg) + 1000;
	char *message = new char[message_len];
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		snprintf(message, message_len, "ERROR: [%s] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		snprintf(message, message_len, "WARNING: [%s] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
	else {
		delete[] message;
		return false;
	}

#ifdef WIN32
	OutputDebugStringA(message);
#else
	fprintf(stderr, "%s\n", message);
#endif
	delete[] message;
	return false;
}
#endif

void vkInstance::instanceInit(const char *appName, const vector<const char *> &enabledExtensions)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.pEngineName = "very lastest engine ever";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();

#ifndef NDEBUG
	// instanceCreateInfo.ppEnabledLayerNames = validationLayerNames;
	// instanceCreateInfo.enabledLayerCount = ARRAY_SIZE(validationLayerNames);
#endif

	VkResult err = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance::instance);
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
		throw runtime_error("Your GPU is from Hønefoss!");
	assumeSuccess(err);

	instanceFuncsInit(vkInstance::instance);

#ifndef NDEBUG
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
	debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debugReportCallbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback;
	debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	assumeSuccess(instanceFuncs.vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackCreateInfo,
	                                                           nullptr, &debugReportCallback));

	// SELF-TEST:
	// instanceFuncs.vkDebugReportMessageEXT(instance, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, nullptr, 0, 0, "self-test", "This is a dummy warning");
#endif
}

static uint32_t findQueue(VkPhysicalDevice physicalDevice, VkQueueFlags requiredFlags, function<bool(VkInstance, VkPhysicalDevice, uint32_t)> usableQueue)
{
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
	assert(queueCount > 0);

	VkQueueFamilyProperties *props = new VkQueueFamilyProperties[queueCount];
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, props);

	for (uint32_t i = 0; i < queueCount; i++) {
		if ((props[i].queueFlags & requiredFlags) == requiredFlags && usableQueue(vkInstance::instance, physicalDevice, i)) {
			delete[] props;
			return i;
		}
	}

	delete[] props;
	throw runtime_error("failed to find queue!");
}

void vkInstance::deviceInit(VkPhysicalDevice physicalDevice, function<bool(VkInstance, VkPhysicalDevice, uint32_t)> usableQueue)
{
	vkInstance::physicalDevice = physicalDevice;

	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

	enabledFeatures.samplerAnisotropy = physicalDeviceFeatures.samplerAnisotropy;

	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	graphicsQueueIndex = findQueue(physicalDevice, VK_QUEUE_GRAPHICS_BIT, usableQueue);

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	float queuePriorities = 0.0f;
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriorities;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

	const char *enabledExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	deviceCreateInfo.enabledExtensionCount = ARRAY_SIZE(enabledExtensions);
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

	assumeSuccess(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
	vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);

	setupCommandPool = createCommandPool(device, graphicsQueueIndex);
}

template <typename T>
static T getDeviceProc(VkDevice device, const char *entrypoint)
{
	auto ret = reinterpret_cast<T>(vkGetDeviceProcAddr(device, entrypoint));
	assert(ret != nullptr);
	return ret;
}

struct vkInstance::instance_funcs vkInstance::instanceFuncs;

template <typename T>
static T getInstanceProc(VkInstance instance, const char *entrypoint)
{
	auto ret = reinterpret_cast<T>(vkGetInstanceProcAddr(instance, entrypoint));
	assert(ret != nullptr);
	return ret;
}

void vkInstance::instanceFuncsInit(VkInstance instance)
{
	instanceFuncs.vkCreateDebugReportCallbackEXT = getInstanceProc<PFN_vkCreateDebugReportCallbackEXT>(instance, "vkCreateDebugReportCallbackEXT");
	instanceFuncs.vkDestroyDebugReportCallbackEXT = getInstanceProc<PFN_vkDestroyDebugReportCallbackEXT>(instance, "vkDestroyDebugReportCallbackEXT");
	instanceFuncs.vkDebugReportMessageEXT = getInstanceProc<PFN_vkDebugReportMessageEXT>(instance, "vkDebugReportMessageEXT");
	instanceFuncs.vkSetDebugUtilsObjectNameEXT = getInstanceProc<PFN_vkSetDebugUtilsObjectNameEXT>(instance, "vkSetDebugUtilsObjectNameEXT");
}
