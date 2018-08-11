#include "texture.h"

using namespace vulkan;

TextureBase::TextureBase(VkFormat format, VkImageType imageType, VkImageViewType imageViewType, int width, int height, int depth, int mipLevels, int arrayLayers, bool useStaging) :
	baseWidth(width),
	baseHeight(height),
	baseDepth(depth),
	mipLevels(mipLevels),
	arrayLayers(arrayLayers)
{
	assert(0 < width && (uint32_t)width <= UINT32_MAX);
	assert(0 < height && (uint32_t)height <= UINT32_MAX);
	assert(0 < depth && (uint32_t)depth <= UINT32_MAX);
	assert(mipLevels > 0);
	assert(arrayLayers > 0);

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE)
		imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	else if (imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
		imageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;

	imageCreateInfo.imageType = imageType;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { (uint32_t)width, (uint32_t)height, (uint32_t)depth };
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = arrayLayers;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = useStaging ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = useStaging ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;

	if (useStaging)
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	assumeSuccess(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, image, &memoryRequirements);

	auto memoryTypeIndex = getMemoryTypeIndex(memoryRequirements, useStaging ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	deviceMemory = allocateDeviceMemory(memoryRequirements.size, memoryTypeIndex);

	assumeSuccess(vkBindImageMemory(device, image, deviceMemory, 0));

	VkImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = arrayLayers;

	imageView = createImageView(image, imageViewType, format, subresourceRange);
}

void TextureBase::uploadFromStagingBuffer(StagingBuffer *stagingBuffer, int mipLevel, int arrayLayer)
{
	assert(stagingBuffer != nullptr);

	auto commandBuffer = allocateCommandBuffers(setupCommandPool, 1)[0];

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	assumeSuccess(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	VkImageSubresourceRange subresourceRange = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		uint32_t(mipLevel), 1,
		uint32_t(arrayLayer), 1
	};

	imageBarrier(commandBuffer,
		image, subresourceRange,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.baseArrayLayer = arrayLayer;
	copyRegion.imageSubresource.mipLevel = mipLevel;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent.width = mipSize(baseWidth, mipLevel);
	copyRegion.imageExtent.height = mipSize(baseHeight, mipLevel);
	copyRegion.imageExtent.depth = mipSize(baseDepth, mipLevel);

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->getBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	assumeSuccess(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	assumeSuccess(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
}
