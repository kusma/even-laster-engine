#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "../vkhelpers.h"

class RenderTargetBase {
protected:
	RenderTargetBase(VkFormat format,
	                 VkImageType imageType, VkImageViewType imageViewType,
	                 int width, int height, int depth,
	                 int arrayLayers, int mipLevels,
	                 VkSampleCountFlagBits sampleCount,
	                 VkImageUsageFlags usage,
	                 VkImageAspectFlags aspect) :
		format(format),
		width(width),
		height(height),
		depth(depth),
		arrayLayers(arrayLayers),
		sampleCount(sampleCount)
	{
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = imageType;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { (uint32_t)width, (uint32_t)height, (uint32_t)depth };
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = arrayLayers;
		imageCreateInfo.samples = sampleCount;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		assumeSuccess(vkCreateImage(vkInstance::device, &imageCreateInfo, nullptr, &image));

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(vkInstance::device, image, &memoryRequirements);

		uint32_t memoryTypeIndex;
		if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
			memoryTypeIndex = findMemoryTypeIndex(vkInstance::deviceMemoryProperties, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			if (memoryTypeIndex >= VK_MAX_MEMORY_TYPES)
				memoryTypeIndex = getMemoryTypeIndex(vkInstance::deviceMemoryProperties, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		} else
			memoryTypeIndex = getMemoryTypeIndex(vkInstance::deviceMemoryProperties, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto deviceMemory = allocateDeviceMemory(vkInstance::device, memoryRequirements.size, memoryTypeIndex);

		assumeSuccess(vkBindImageMemory(vkInstance::device, image, deviceMemory, 0));

		VkImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask = aspect;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = arrayLayers;

		imageView = createImageView(vkInstance::device, image, imageViewType, format, subresourceRange);
	}

public:
	VkFormat getFormat() { return format; }

	int getWidth() const { return width; }
	int getHeight() const { return height; }
	int getDepth() const { return depth; }

	int getArrayLayers() const { return arrayLayers; }

	VkImage getImage() { return image; }
	VkImageView getImageView() { return imageView; }

	VkSampleCountFlagBits getSampleCount() { return sampleCount; }

protected:
	VkFormat format;

	int width, height, depth;
	int arrayLayers;

	VkSampleCountFlagBits sampleCount;

	VkImage image;
	VkImageView imageView;
};

class ColorRenderTarget : public RenderTargetBase {
public:
	ColorRenderTarget(VkFormat format, int width, int height,
	                  int mipLevels = 1,
	                  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	                  VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) :
		RenderTargetBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, width, height, 1, 1, mipLevels, sampleCount, usage, VK_IMAGE_ASPECT_COLOR_BIT)
	{
	}
};

class DepthRenderTarget : public RenderTargetBase {
public:
	DepthRenderTarget(VkFormat format, int width, int height,
	                  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	                  VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) :
		RenderTargetBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, width, height, 1, 1, 1, sampleCount, usage, VK_IMAGE_ASPECT_DEPTH_BIT)
	{
	}
};

class Texture2DArrayRenderTarget : public RenderTargetBase {
public:
	Texture2DArrayRenderTarget(VkFormat format, int width, int height, int arrayLayers,
	                           VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	                           VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) :
		RenderTargetBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY, width, height, 1, arrayLayers, 1, sampleCount, usage, VK_IMAGE_ASPECT_COLOR_BIT)
	{
		arrayImageViews.reserve(arrayLayers);
		for (int i = 0; i < arrayLayers; ++i) {
			VkImageSubresourceRange subresourceRange;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = i;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			arrayImageViews.push_back(createImageView(vkInstance::device, image, VK_IMAGE_VIEW_TYPE_2D, format, subresourceRange));
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
