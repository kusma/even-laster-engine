#include "texture.h"

#include "vkhelpers.h"
#include "buffer.h"

using namespace vkHelpers;

TextureBase::TextureBase(VkFormat format, VkImageType imageType, VkImageViewType imageViewType,
                         unsigned width, unsigned height, unsigned depth, unsigned mipLevels, unsigned arrayLayers,
                         VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags) :
	baseWidth(width),
	baseHeight(height),
	baseDepth(depth),
	mipLevels(mipLevels),
	arrayLayers(arrayLayers)
{
	assert(width <= UINT32_MAX);
	assert(height <= UINT32_MAX);
	assert(depth <= UINT32_MAX);

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE)
		imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	else if (imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY && imageType == VK_IMAGE_TYPE_3D)
		imageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;

	imageCreateInfo.imageType = imageType;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { width, height, depth };
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = arrayLayers;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocInfo = {
		.flags = allocFlags,
		.usage = memoryUsage,
	};

	assumeSuccess(vmaCreateImage(vkInstance::allocator, &imageCreateInfo, &allocInfo, &image, &allocation, nullptr));

	VkImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = arrayLayers;

	imageView = createImageView(vkInstance::device, image, imageViewType, format, subresourceRange);
}

void TextureBase::uploadFromStagingBuffer(StagingBuffer *stagingBuffer,
                                          unsigned mipLevel, unsigned arrayLayer)
{
	assert(stagingBuffer != nullptr);

	auto commandBuffer = vkInstance::getSetupCommandBuffer();

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	assumeSuccess(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	VkImageSubresourceRange subresourceRange = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		mipLevel, 1,
		arrayLayer, 1
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

	assumeSuccess(vkQueueSubmit(vkInstance::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
}
