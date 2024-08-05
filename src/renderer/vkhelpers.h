#ifndef VULKAN_H
#define VULKAN_H

#include <volk.h>

#include <algorithm>
#include <vector>
#include <cassert>
#include <stdexcept>

#include "core/core.h"

namespace vkHelpers
{
	inline void assumeSuccess(VkResult result)
	{
		// Success codes are non-negative
		assert(result >= 0);

		// failures are negative
		if (result < 0)
			throw std::runtime_error("unexpected return code");
	}

	inline VkDeviceSize alignSize(VkDeviceSize value, VkDeviceSize alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}

	inline VkSampleCountFlagBits getMaxMSAACount(VkPhysicalDeviceProperties physicalDeviceProperties) {
		VkSampleCountFlags supportedCounts =
			physicalDeviceProperties.limits.framebufferColorSampleCounts &
			physicalDeviceProperties.limits.framebufferDepthSampleCounts;

#define CHECK(x) \
		if (supportedCounts & VK_SAMPLE_COUNT_ ## x ## _BIT) \
			return VK_SAMPLE_COUNT_ ## x ## _BIT;
		CHECK(32)
		CHECK(16)
		CHECK(8)
		CHECK(4)
		CHECK(2)

		throw std::runtime_error("no supported msaa-count!");
	}

	inline std::vector<VkCommandBuffer> allocateCommandBuffers(VkDevice device, VkCommandPool commandPool, unsigned commandBufferCount)
	{
		VkCommandBufferAllocateInfo commandAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = commandBufferCount,
		};

		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(commandBufferCount);
		assumeSuccess(vkAllocateCommandBuffers(device, &commandAllocInfo, commandBuffers.data()));

		return commandBuffers;
	}

	inline VkFormat findBestFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (auto format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			switch (tiling) {
			case VK_IMAGE_TILING_LINEAR:
				if ((props.linearTilingFeatures & features) == features)
					return format;
				break;
			case VK_IMAGE_TILING_OPTIMAL:
				if ((props.optimalTilingFeatures & features) == features)
					return format;
				break;
			default:
				unreachable("unexpected tiling mode");
			}
		}

		throw std::runtime_error("no supported format!");
	}

	inline VkFence createFence(VkDevice device, VkFenceCreateFlags flags)
	{
		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = flags,
		};

		VkFence ret;
		assumeSuccess(vkCreateFence(device, &fenceCreateInfo, nullptr, &ret));
		return ret;
	}

	inline VkSemaphore createSemaphore(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkSemaphore ret;
		assumeSuccess(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &ret));
		return ret;
	}

	inline void setViewport(VkCommandBuffer commandBuffer, float x, float y, float width, float height)
	{
		VkViewport viewport = {
			.x = x,
			.y = y,
			.width = width,
			.height = height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	inline void setScissor(VkCommandBuffer commandBuffer,
	                       int x, int y, unsigned width, unsigned height)
	{
		VkRect2D scissor = {
			.offset = {x, y},
			.extent = {width, height},
		};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	inline void imageBarrier(VkCommandBuffer commandBuffer, VkImage image,
		const VkImageSubresourceRange &subresourceRange,
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccess, VkAccessFlags dstAccess,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		uint32_t oldQueueFamily = VK_QUEUE_FAMILY_IGNORED,
		uint32_t newQueueFamily = VK_QUEUE_FAMILY_IGNORED)
	{
		VkImageMemoryBarrier imageBarrier = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			srcAccess,
			dstAccess,
			oldLayout,
			newLayout,
			oldQueueFamily,
			newQueueFamily,
			image,
			subresourceRange
		};

		vkCmdPipelineBarrier(
			commandBuffer, srcStage, dstStage, 0,
			0, nullptr,
			0, nullptr,
			1, &imageBarrier
		);
	}

	inline void imageBarrier(VkCommandBuffer commandBuffer, VkImage image,
		VkImageAspectFlags imageAspectFlags,
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccess, VkAccessFlags dstAccess,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		uint32_t oldQueueFamily = VK_QUEUE_FAMILY_IGNORED,
		uint32_t newQueueFamily = VK_QUEUE_FAMILY_IGNORED)
	{
		VkImageSubresourceRange subresourceRange = {
			imageAspectFlags,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		};

		imageBarrier(commandBuffer,
			image, subresourceRange,
			srcStage, dstStage,
			srcAccess, dstAccess,
			oldLayout, newLayout,
			oldQueueFamily, newQueueFamily);
	}

	inline void blitImage(
		VkCommandBuffer commandBuffer,
		VkImage srcImage, VkImage dstImage,
		const std::vector<VkImageBlit> &imageBlits,
		VkFilter filter = VK_FILTER_NEAREST,
		VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		vkCmdBlitImage(commandBuffer,
			srcImage, srcLayout,
			dstImage, dstLayout,
			imageBlits.size(), imageBlits.data(),
			filter);
	}

	inline void blitImage(
		VkCommandBuffer commandBuffer,
		VkImage srcImage, VkImage dstImage,
		int srcWidth, int srcHeight,
		int dstWidth, int dstHeight,
		VkImageSubresourceLayers srcSubresourceLayers,
		VkImageSubresourceLayers dstSubresourceLayers,
		VkFilter filter = VK_FILTER_LINEAR,
		VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		VkImageBlit imageBlit = {
			.srcSubresource = srcSubresourceLayers,
			.srcOffsets = {
				{ 0, 0, 0 },
				{ srcWidth, srcHeight, 1 },
			},
			.dstSubresource = dstSubresourceLayers,
			.dstOffsets = {
				{ 0, 0, 0 },
				{ dstWidth, dstHeight, 1 },
			},
		};

		blitImage(commandBuffer,
			srcImage, dstImage,
			{ imageBlit }, filter,
			srcLayout, dstLayout);
	}

	inline void blitImage(
		VkCommandBuffer commandBuffer,
		VkImage srcImage, VkImage dstImage,
		int srcWidth, int srcHeight,
		const VkRect2D &dstRect,
		VkImageSubresourceLayers srcSubresourceLayers,
		VkImageSubresourceLayers dstSubresourceLayers,
		VkFilter filter = VK_FILTER_LINEAR,
		VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		VkImageBlit imageBlit = {
			.srcSubresource = srcSubresourceLayers,
			.srcOffsets = {
				{ 0, 0, 0 },
				{ srcWidth, srcHeight, 1 },
			},
			.dstSubresource = dstSubresourceLayers,
			.dstOffsets = {
				{ dstRect.offset.x, dstRect.offset.y, 0 },
				{ int32_t(dstRect.offset.x + dstRect.extent.width),
				  int32_t(dstRect.offset.y + dstRect.extent.height),
				  1 },
			},
		};

		blitImage(commandBuffer,
			srcImage, dstImage,
			{ imageBlit }, filter,
			srcLayout, dstLayout);
	}

	inline void blitImage(
		VkCommandBuffer commandBuffer,
		VkImage srcImage, VkImage dstImage,
		int width, int height,
		VkImageSubresourceLayers srcSubresourceLayers,
		VkImageSubresourceLayers dstSubresourceLayers,
		VkFilter filter = VK_FILTER_NEAREST,
		VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		blitImage(
			commandBuffer,
			srcImage, dstImage,
			width, height,
			width, height,
			srcSubresourceLayers,
			dstSubresourceLayers,
			filter,
			srcLayout, dstLayout);
	}

	inline VkDescriptorPool createDescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize> &poolSizes, unsigned maxSets)
	{
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = maxSets,
			.poolSizeCount = uint32_t(poolSizes.size()),
			.pPoolSizes = poolSizes.data(),
		};

		VkDescriptorPool descriptorPool;
		assumeSuccess(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
		return descriptorPool;
	}

	inline VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIndex)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndex,
		};

		VkCommandPool commandPool;
		assumeSuccess(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));
		return commandPool;
	}

	inline VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptorSetLayout,
		};

		VkDescriptorSet descriptorSet;
		assumeSuccess(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));
		return descriptorSet;
	}

#define IDENTITY_SWIZZLE    \
{                           \
	VK_COMPONENT_SWIZZLE_R, \
	VK_COMPONENT_SWIZZLE_G, \
	VK_COMPONENT_SWIZZLE_B, \
	VK_COMPONENT_SWIZZLE_A  \
}

	inline VkImageView createImageView(VkDevice device, VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange &subresourceRange, VkComponentMapping components = IDENTITY_SWIZZLE)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = viewType,
			.format = format,
			.components = components,
			.subresourceRange = subresourceRange,
		};

		VkImageView imageView;
		assumeSuccess(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));
		return imageView;
	}

	inline VkSampler createSampler(VkDevice device, VkPhysicalDeviceFeatures deviceFeatures, VkPhysicalDeviceProperties deviceProperties, float maxLod, bool repeat, bool wantAnisotropy)
	{
		VkSamplerCreateInfo samplerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0.0f,
			.maxLod = maxLod,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
		};

		if (wantAnisotropy && deviceFeatures.samplerAnisotropy) {
			samplerCreateInfo.maxAnisotropy = std::min(8.0f, deviceProperties.limits.maxSamplerAnisotropy);
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
		}

		VkSampler textureSampler;
		assumeSuccess(vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureSampler));
		return textureSampler;
	}

	inline VkFramebuffer createFramebuffer(VkDevice device, unsigned width, unsigned height, unsigned layers, std::vector<VkImageView> attachments, VkRenderPass renderPass)
	{
		assert(attachments.size() > 0);

		VkFramebufferCreateInfo framebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = uint32_t(attachments.size()),
			.pAttachments = attachments.data(),
			.width = width,
			.height = height,
			.layers = layers,
		};

		VkFramebuffer framebuffer;
		assumeSuccess(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer));
		return framebuffer;
	}

	inline VkPipelineLayout createPipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts, const std::vector<VkPushConstantRange> &pushConstantRanges)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = uint32_t(descriptorSetLayouts.size()),
			.pSetLayouts = descriptorSetLayouts.data(),
			.pushConstantRangeCount = uint32_t(pushConstantRanges.size()),
			.pPushConstantRanges = pushConstantRanges.data(),
		};

		VkPipelineLayout pipelineLayout;
		assumeSuccess(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
		return pipelineLayout;
	}

	inline VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &layoutBindings, VkDescriptorSetLayoutCreateFlags flags = 0)
	{
		VkDescriptorSetLayoutCreateInfo desciptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.flags = flags,
			.bindingCount = uint32_t(layoutBindings.size()),
			.pBindings = layoutBindings.data(),
		};

		VkDescriptorSetLayout descriptorSetLayout;
		assumeSuccess(vkCreateDescriptorSetLayout(device, &desciptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
		return descriptorSetLayout;
	}

	inline void setImageName(VkDevice device, VkImage image, const std::string &name)
	{
#ifndef NDEBUG
/*
		const VkDebugUtilsObjectNameInfoEXT imageNameInfo =
		{
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			NULL,
			VK_OBJECT_TYPE_IMAGE,
			(uint64_t)image,
			name.c_str(),
		};

		// HACK!
		// vkSetDebugUtilsObjectNameEXT(device, &imageNameInfo);
*/
#endif
	}

	struct ShaderStages {
		VkShaderModule vertexShader, geometryShader, fragmentShader;
	};

	inline std::vector<VkPipelineShaderStageCreateInfo> createStageVector(const ShaderStages &stages)
	{
		assert(stages.vertexShader != VK_NULL_HANDLE);

		std::vector<VkPipelineShaderStageCreateInfo> ret = { {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = stages.vertexShader,
				.pName = "main"
			} };

		if (stages.geometryShader)
			ret.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_GEOMETRY_BIT,
				.module = stages.geometryShader,
				.pName = "main",
			});

		if (stages.fragmentShader)
			ret.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = stages.fragmentShader,
				.pName = "main",
			});

		return ret;
	}

	inline void updateCombinedImageDescriptor(VkDevice device,
	                                          VkDescriptorSet descriptorSet,
	                                          unsigned dstBinding,
	                                          const std::vector<VkDescriptorImageInfo> &imageInfo)
	{
		assert(imageInfo.size() < UINT32_MAX);
		VkWriteDescriptorSet writeDescriptorSets = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = dstBinding,
			.descriptorCount = uint32_t(imageInfo.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = imageInfo.data(),
		};
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSets, 0, nullptr);
	}

	inline void updateStorageImageDescriptor(VkDevice device,
	                                         VkDescriptorSet descriptorSet,
	                                         unsigned dstBinding,
	                                         const std::vector<VkDescriptorImageInfo> &imageInfo)
	{
		assert(imageInfo.size() < UINT32_MAX);
		VkWriteDescriptorSet writeDescriptorSets = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = dstBinding,
			.descriptorCount = uint32_t(imageInfo.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = imageInfo.data(),
		};
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSets, 0, nullptr);
	}

	inline void writeUniformBufferDescriptor(VkDevice device,
	                                         VkDescriptorSet descriptorSet,
	                                         unsigned dstBinding,
	                                         const std::vector<VkDescriptorBufferInfo> &bufferInfo)
	{
		assert(bufferInfo.size() < UINT32_MAX);
		VkWriteDescriptorSet writeDescriptorSets = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = dstBinding,
			.descriptorCount = uint32_t(bufferInfo.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = bufferInfo.data(),
		};
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSets, 0, nullptr);
	}
};

#endif // VULKAN_H
