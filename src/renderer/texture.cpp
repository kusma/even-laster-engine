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

	VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = 0,
		.imageType = imageType,
		.format = format,
		.extent = { width, height, depth },
		.mipLevels = mipLevels,
		.arrayLayers = arrayLayers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE)
		imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	else if (imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY && imageType == VK_IMAGE_TYPE_3D)
		imageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;

	VmaAllocationCreateInfo allocInfo = {
		.flags = allocFlags,
		.usage = memoryUsage,
	};

	assumeSuccess(vmaCreateImage(vkInstance::allocator, &imageCreateInfo, &allocInfo, &image, &allocation, nullptr));

	VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = mipLevels,
		.baseArrayLayer = 0,
		.layerCount = arrayLayers,
	};

	imageView = createImageView(vkInstance::device, image, imageViewType, format, subresourceRange);
}

void TextureBase::uploadFromStagingBuffer(VkCommandBuffer commandBuffer,
                                          StagingBuffer *stagingBuffer,
                                          unsigned mipLevel, unsigned arrayLayer)
{
	assert(stagingBuffer != nullptr);
	VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = mipLevel,
		.levelCount = 1,
		.baseArrayLayer = arrayLayer,
		.layerCount = 1,
	};

	imageBarrier(commandBuffer,
		image, subresourceRange,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy copyRegion = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = mipLevel,
			.baseArrayLayer = arrayLayer,
			.layerCount = 1,
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = {
			.width = mipSize(baseWidth, mipLevel),
			.height = mipSize(baseHeight, mipLevel),
			.depth = mipSize(baseDepth, mipLevel),
		},
	};

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->getBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	imageBarrier(commandBuffer,
		image, subresourceRange,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
