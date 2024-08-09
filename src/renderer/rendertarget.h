#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "vkhelpers.h"

class RenderTargetBase {
protected:
	RenderTargetBase(VkFormat format,
	                 VkImageType imageType, VkImageViewType imageViewType,
	                 unsigned width, unsigned height, unsigned depth,
	                 unsigned arrayLayers, unsigned mipLevels,
	                 VkSampleCountFlagBits sampleCount,
	                 VkImageUsageFlags usage,
	                 VkImageAspectFlags aspect,
	                 VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags) :
		format(format),
		width(width),
		height(height),
		depth(depth),
		arrayLayers(arrayLayers)
	{
		VkImageCreateInfo imageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = 0,
			.imageType = imageType,
			.format = format,
			.extent = { width, height, depth },
			.mipLevels = mipLevels,
			.arrayLayers = arrayLayers,
			.samples = sampleCount,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VmaAllocationCreateInfo allocInfo = {
			.flags = allocFlags,
			.usage = memoryUsage,
		};

		if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
			allocInfo. preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

		vkHelpers::assumeSuccess(vmaCreateImage(vkInstance::allocator, &imageCreateInfo, &allocInfo, &image, &allocation, nullptr));

		VkImageSubresourceRange subresourceRange = {
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = arrayLayers,
		};

		imageView = vkHelpers::createImageView(vkInstance::device, image, imageViewType, format, subresourceRange);
	}

public:
	VkFormat getFormat() { return format; }

	unsigned getWidth() const { return width; }
	unsigned getHeight() const { return height; }
	unsigned getDepth() const { return depth; }

	unsigned getArrayLayers() const { return arrayLayers; }

	VkImage getImage() { return image; }
	VkImageView getImageView() { return imageView; }

protected:
	VkFormat format;

	unsigned width, height, depth;
	unsigned arrayLayers;

	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
};

class ColorRenderTarget : public RenderTargetBase {
public:
	ColorRenderTarget(VkFormat format, unsigned width, unsigned height,
	                  unsigned mipLevels = 1,
	                  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	                  VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) :
		RenderTargetBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D,
		                 width, height, 1, 1, mipLevels,
		                 sampleCount, usage,
		                 VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class DepthRenderTarget : public RenderTargetBase {
public:
	DepthRenderTarget(VkFormat format, unsigned width, unsigned height,
	                  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	                  VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) :
		RenderTargetBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D,
		                 width, height, 1, 1, 1,
		                 sampleCount, usage, VK_IMAGE_ASPECT_DEPTH_BIT,
		                 VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class Texture2DArrayRenderTarget : public RenderTargetBase {
public:
	Texture2DArrayRenderTarget(VkFormat format, unsigned width, unsigned height, unsigned arrayLayers,
	                           VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	                           VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) :
		RenderTargetBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		                 width, height, 1, arrayLayers, 1,
		                 sampleCount, usage, VK_IMAGE_ASPECT_COLOR_BIT,
		                 VMA_MEMORY_USAGE_AUTO, 0)
	{
		arrayImageViews.reserve(arrayLayers);
		for (unsigned i = 0; i < arrayLayers; ++i) {
			VkImageSubresourceRange subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = i,
				.layerCount = 1,
			};
			arrayImageViews.push_back(vkHelpers::createImageView(vkInstance::device, image, VK_IMAGE_VIEW_TYPE_2D, format, subresourceRange));
		}
	}

	const std::vector<VkImageView> &getArrayImageViews() const
	{
		return arrayImageViews;
	}

private:
	std::vector<VkImageView> arrayImageViews;
};

#endif // RENDERTARGET_H
