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

void Buffer::uploadFromStagingBuffer(VkCommandBuffer commandBuffer, StagingBuffer *stagingBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
	assert(stagingBuffer != nullptr);
	VkBufferCopy bufferCopy = {
		.srcOffset = srcOffset,
		.dstOffset = dstOffset,
		.size = size,
	};
	vkCmdCopyBuffer(commandBuffer, stagingBuffer->getBuffer(), buffer, 1, &bufferCopy);
}
