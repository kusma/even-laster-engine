#ifndef BUFFER_H
#define BUFFER_H

#include "vkhelpers.h"
#include "vkinstance.h"

#include <cstring>

class StagingBuffer;

class Buffer {
public:
	Buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags);
	~Buffer();

	void *map()
	{
		void *ret;
		vkHelpers::assumeSuccess(vmaMapMemory(vkInstance::allocator, allocation, &ret));
		return ret;
	}

	void unmap()
	{
		vmaUnmapMemory(vkInstance::allocator, allocation);
	}

	void uploadMemory(void *data, VkDeviceSize size)
	{
		auto mappedUniformMemory = map();
		memcpy(mappedUniformMemory, data, (size_t)size);
		unmap();
	}

	VkBuffer getBuffer() const { return buffer; }
	VkDeviceSize getSize() const { return size; }

	VkDescriptorBufferInfo getDescriptorBufferInfo(VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE)
	{
		return {
			.buffer = buffer,
			.offset = offset,
			.range = range,
		};
	}

	void uploadFromStagingBuffer(VkCommandBuffer commandBuffer, StagingBuffer *stagingBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);

private:
	VkBuffer buffer;
	VkDeviceSize size;
	VmaAllocation allocation;
};

class StagingBuffer : public Buffer {
public:
	StagingBuffer(VkDeviceSize size) : Buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
	{
	}
};

class UniformBuffer : public Buffer {
public:
	UniformBuffer(VkDeviceSize size) : Buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class VertexBuffer : public Buffer {
public:
	VertexBuffer(VkDeviceSize size) : Buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class IndexBuffer : public Buffer {
public:
	IndexBuffer(VkDeviceSize size) : Buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

#endif // BUFFER_H
