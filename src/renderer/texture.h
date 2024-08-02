#ifndef TEXTURE_H
#define TEXTURE_H

#include <algorithm>
#include <cassert>

#include "vkinstance.h"
#include "vkhelpers.h"

class StagingBuffer;

class TextureBase {
protected:
	TextureBase(VkFormat format, VkImageType imageType, VkImageViewType imageViewType,
	            unsigned width, unsigned height, unsigned depth, unsigned mipLevels, unsigned arrayLayers,
	            VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags);

public:

	static unsigned mipSize(unsigned size, unsigned mipLevel)
	{
		return std::max(size >> mipLevel, 1u);
	}

	unsigned getWidth(unsigned level = 0) { return mipSize(baseWidth, level); }
	unsigned getHeight(unsigned level = 0) { return mipSize(baseHeight, level); }
	unsigned getDepth(unsigned level = 0) { return mipSize(baseDepth, level); }

	unsigned getMipLevels() const { return mipLevels; }
	unsigned getArrayLayers() const { return arrayLayers; }

	void uploadFromStagingBuffer(VkCommandBuffer commandBuffer,
	                             StagingBuffer *stagingBuffer,
	                             unsigned mipLevel = 0, unsigned arrayLayer = 0);

	VkImageView getImageView() const
	{
		return imageView;
	}

	VkImage getImage() const
	{
		return image;
	}

	VkSubresourceLayout getSubresourceLayout(unsigned mipLevel = 0, unsigned arrayLayer = 0)
	{
		VkImageSubresource subRes = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = mipLevel,
			.arrayLayer = arrayLayer,
		};

		VkSubresourceLayout ret;
		vkGetImageSubresourceLayout(vkInstance::device, image, &subRes, &ret);
		return ret;
	}

	VkDescriptorImageInfo getDescriptorImageInfo(VkSampler textureSampler)
	{
		return {
			.sampler = textureSampler,
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // TODO: make *sure* of this!
		};
	}

protected:
	unsigned baseWidth, baseHeight, baseDepth;
	unsigned mipLevels, arrayLayers;

	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
};

class Texture2D : public TextureBase {
public:
	Texture2D(VkFormat format, unsigned width, unsigned height, unsigned mipLevels = 1, unsigned arrayLayers = 1) :
		TextureBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, width, height, 1, mipLevels, arrayLayers, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class Texture2DArray : public TextureBase {
public:
	Texture2DArray(VkFormat format, unsigned width, unsigned height, unsigned arrayLayers, unsigned mipLevels = 1) :
		TextureBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY, width, height, 1, mipLevels, arrayLayers, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class TextureCube : public TextureBase {
public:
	TextureCube(VkFormat format, unsigned size, unsigned mipLevels = 1) :
		TextureBase(format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_CUBE, size, size, 1, mipLevels, 6, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

class Texture3D : public TextureBase {
public:
	Texture3D(VkFormat format, unsigned width, unsigned height, unsigned depth, unsigned mipLevels = 1) :
		TextureBase(format, VK_IMAGE_TYPE_3D, VK_IMAGE_VIEW_TYPE_3D, width, height, depth, mipLevels, 1, VMA_MEMORY_USAGE_AUTO, 0)
	{
	}
};

#endif // TEXTURE_H
