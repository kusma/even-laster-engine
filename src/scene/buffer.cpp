#include "buffer.h"

using namespace vulkan;

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags) :
	size(size)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;

	assumeSuccess(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	auto memoryTypeIndex = getMemoryTypeIndex(memoryRequirements, memoryPropertyFlags);
	deviceMemory = allocateDeviceMemory(memoryRequirements.size, memoryTypeIndex);

	assumeSuccess(vkBindBufferMemory(device, buffer, deviceMemory, 0));
}

Buffer::~Buffer()
{
	vkDestroyBuffer(device, buffer, nullptr);
	vkFreeMemory(device, deviceMemory, nullptr);
}

void Buffer::uploadFromStagingBuffer(StagingBuffer *stagingBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
	assert(stagingBuffer != nullptr);

	auto commandBuffer = allocateCommandBuffers(setupCommandPool, 1)[0];

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	assumeSuccess(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	VkBufferCopy bufferCopy = {};
	bufferCopy.srcOffset = srcOffset;
	bufferCopy.dstOffset = dstOffset;
	bufferCopy.size = size;
	vkCmdCopyBuffer(commandBuffer, stagingBuffer->getBuffer(), buffer, 1, &bufferCopy);

	assumeSuccess(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	assumeSuccess(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
}
