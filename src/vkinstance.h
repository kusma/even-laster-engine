#include <volk.h>

#include <vector>
#include <functional>

#include <cstdint>

namespace vkInstance
{
	extern VkInstance instance;
	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkPhysicalDeviceFeatures enabledFeatures;
	extern VkPhysicalDeviceProperties deviceProperties;
	extern VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	extern VkQueue graphicsQueue;
	extern uint32_t graphicsQueueIndex;

	extern VkCommandPool setupCommandPool;

	extern VkDebugReportCallbackEXT debugReportCallback;

	void instanceInit(const char *appName, const std::vector<const char *> &enabledExtensions);
	void deviceInit(VkPhysicalDevice physicalDevice, std::function<bool(VkInstance, VkPhysicalDevice, uint32_t)> usableQueue);
}
