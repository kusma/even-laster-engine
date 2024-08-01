#include "buffer.h"
#include "vkhelpers.h"

using namespace vkHelpers;

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags) :
	size(size)
{
	VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usageFlags,
	};
	VmaAllocationCreateInfo allocInfo = {
		.flags = allocFlags,
		.usage = memoryUsage,
	};
	assumeSuccess(vmaCreateBuffer(vkInstance::allocator, &bufferCreateInfo, &allocInfo, &buffer, &allocation, nullptr));
}

Buffer::~Buffer()
{
	vkDestroyBuffer(vkInstance::device, buffer, nullptr);
	vmaFreeMemory(vkInstance::allocator, allocation);
}

void Buffer::uploadFromStagingBuffer(StagingBuffer *stagingBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
	assert(stagingBuffer != nullptr);

	auto commandBuffer = vkInstance::getSetupCommandBuffer();

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

	assumeSuccess(vkQueueSubmit(vkInstance::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
}
