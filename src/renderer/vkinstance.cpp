#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "vkinstance.h"

#include "buffer.h"

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
VmaAllocator vkInstance::allocator;
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

	assert(0);

	return false;
}
#endif

void vkInstance::instanceInit(const char *appName, const vector<const char *> &enabledExtensions)
{
	VkResult err = volkInitialize();
	assumeSuccess(err);

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = appName,
		.pEngineName = "very lastest engine ever",
		.apiVersion = VK_API_VERSION_1_3,
	};

	VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = uint32_t(enabledExtensions.size()),
		.ppEnabledExtensionNames = enabledExtensions.data(),
	};

	err = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance::instance);
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
		throw runtime_error("Your GPU is from Hønefoss!");
	assumeSuccess(err);

	volkLoadInstance(vkInstance::instance);

#ifndef NDEBUG
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
		.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
		.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback,
	};
	assumeSuccess(vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackCreateInfo,
	                                             nullptr, &debugReportCallback));

	// SELF-TEST:
	// vkDebugReportMessageEXT(instance, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, nullptr, 0, 0, "self-test", "This is a dummy warning");
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
	enabledFeatures.geometryShader = true;

	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	graphicsQueueIndex = findQueue(physicalDevice, VK_QUEUE_GRAPHICS_BIT, usableQueue);

	float queuePriorities = 0.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphicsQueueIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities,
	};

	const char *enabledExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	};

	VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = ARRAY_SIZE(enabledExtensions),
		.ppEnabledExtensionNames = enabledExtensions,
		.pEnabledFeatures = &enabledFeatures,
	};


	assumeSuccess(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

	VmaVulkanFunctions vulkanFunctions = {
#define P(x) .x = x,
	P(vkGetInstanceProcAddr)
	P(vkGetDeviceProcAddr)
	P(vkGetPhysicalDeviceProperties)
	P(vkGetPhysicalDeviceMemoryProperties)
	P(vkAllocateMemory)
	P(vkFreeMemory)
	P(vkMapMemory)
	P(vkUnmapMemory)
	P(vkFlushMappedMemoryRanges)
	P(vkInvalidateMappedMemoryRanges)
	P(vkBindBufferMemory)
	P(vkBindImageMemory)
	P(vkGetBufferMemoryRequirements)
	P(vkGetImageMemoryRequirements)
	P(vkCreateBuffer)
	P(vkDestroyBuffer)
	P(vkCreateImage)
	P(vkDestroyImage)
	P(vkCmdCopyBuffer)
	P(vkGetBufferMemoryRequirements2KHR)
	P(vkGetImageMemoryRequirements2KHR)
	P(vkBindBufferMemory2KHR)
	P(vkBindImageMemory2KHR)
	.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
	P(vkGetDeviceBufferMemoryRequirements)
	P(vkGetDeviceImageMemoryRequirements)
#undef P
	};

	VmaAllocatorCreateInfo allocatorCreateInfo = {
		.flags = 0,
		.physicalDevice = physicalDevice,
		.device = device,
		.pVulkanFunctions = &vulkanFunctions,
		.instance = instance,
		.vulkanApiVersion = deviceProperties.apiVersion,
	};
	vmaCreateAllocator(&allocatorCreateInfo, &vkInstance::allocator);

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
	vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);

	setupCommandPool = createCommandPool(device, graphicsQueueIndex);
}

static std::vector<VkCommandBuffer> setupCommandBuffers;
static std::vector<StagingBuffer*> setupStagingBuffers;

VkCommandBuffer vkInstance::getSetupCommandBuffer()
{
	auto cb = allocateCommandBuffers(device, setupCommandPool, 1)[0];
	setupCommandBuffers.push_back(cb);
	return cb;
}

StagingBuffer *vkInstance::getStagingBuffer(VkDeviceSize size)
{
	auto sb = new StagingBuffer(size);
	setupStagingBuffers.push_back(sb);
	return sb;
}

void vkInstance::finishSetup()
{
	assumeSuccess(vkQueueWaitIdle(vkInstance::graphicsQueue));

	/* free up resources */
	for (auto cb : setupCommandBuffers)
		vkFreeCommandBuffers(device, setupCommandPool, 1, &cb);
	setupCommandBuffers.clear();

	for (auto sb : setupStagingBuffers)
		delete sb;

	setupStagingBuffers.clear();
}

void vkInstance::submitSetupCommands(std::function<void(VkCommandBuffer)> callback)
{
	auto commandBuffer = vkInstance::getSetupCommandBuffer();
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	assumeSuccess(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	callback(commandBuffer);

	assumeSuccess(vkEndCommandBuffer(commandBuffer));
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};
	assumeSuccess(vkQueueSubmit(vkInstance::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
}
