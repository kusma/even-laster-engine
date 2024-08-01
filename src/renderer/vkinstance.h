#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <vector>
#include <functional>

#include <cstdint>

class StagingBuffer;

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
	VkCommandBuffer getSetupCommandBuffer();
	StagingBuffer *getStagingBuffer(VkDeviceSize size);
	void finishSetup();

	extern VmaAllocator allocator;
	extern VkDebugReportCallbackEXT debugReportCallback;

	void instanceInit(const char *appName, const std::vector<const char *> &enabledExtensions);
	void deviceInit(VkPhysicalDevice physicalDevice, std::function<bool(VkInstance, VkPhysicalDevice, uint32_t)> usableQueue);
}
