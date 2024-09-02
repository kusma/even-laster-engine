#ifndef RENDERPASS_H
#define RENDERPASS_H

#include "vkhelpers.h"
#include "vkinstance.h"
#include "rendertarget.h"
#include <cassert>

/*
 *
 * Current Assumptions:
 * - The depth buffer is cleared on start
 * - Depth buffer is ignored
 * - If more than one sample: "Normal" MSAA
 *   - E.g. many samples resolved to one
 *   - Only the resolved color-buffer
*/

class RenderPass {
public:
	RenderPass(VkFormat colorFormat = VK_FORMAT_UNDEFINED,
	           VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED,
	           VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
	           VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	           VkImageLayout colorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED) :
		colorFormat(colorFormat),
		depthStencilFormat(depthStencilFormat),
		sampleCount(sampleCount)
	{
		VkAttachmentReference depthAttachmentReference = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference colorAttachmentReference = {
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference colorResolveAttachmentReference = {
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = nullptr,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
		};

		std::vector<VkAttachmentDescription> attachments;
		if (depthStencilFormat != VK_FORMAT_UNDEFINED) {
			assert(depthAttachmentReference.attachment == 0);
			subpass.pDepthStencilAttachment = &depthAttachmentReference,
			attachments.push_back({
				.flags = 0,
				.format = depthStencilFormat,
				.samples = sampleCount,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			});
		}

		if (colorFormat != VK_FORMAT_UNDEFINED) {
			colorAttachmentReference.attachment = attachments.size();
			subpass.pColorAttachments = &colorAttachmentReference,
			attachments.push_back({
				.flags = 0,
				.format = colorFormat,
				.samples = sampleCount,
				.loadOp = colorLoadOp,
				.storeOp = sampleCount != VK_SAMPLE_COUNT_1_BIT ?
				           VK_ATTACHMENT_STORE_OP_DONT_CARE :
				           VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = colorInitialLayout,
				.finalLayout = sampleCount != VK_SAMPLE_COUNT_1_BIT ?
				               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
				               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			});

			if (sampleCount != VK_SAMPLE_COUNT_1_BIT) {
				colorResolveAttachmentReference.attachment = attachments.size();
				subpass.pResolveAttachments = &colorResolveAttachmentReference;
				attachments.push_back({
					.flags = 0,
					.format = colorFormat,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				});
			}
		}

		std::vector<VkSubpassDependency> dependencies = { {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = 0,
		} };

		VkRenderPassCreateInfo renderPassCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = uint32_t(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = uint32_t(dependencies.size()),
			.pDependencies = dependencies.data(),
		};

		vkHelpers::assumeSuccess(
			vkCreateRenderPass(vkInstance::device, &renderPassCreateInfo,
			                   nullptr, &renderPass));
	}

	VkRenderPass getRenderPass() const { return renderPass; }

	VkFramebuffer createFramebuffer(const std::vector<RenderTargetBase*> &renderTargets) {
		unsigned width = 0, height = 0, layers = 0;

		std::vector<VkImageView> imageViews;
		if (depthStencilFormat != VK_FORMAT_UNDEFINED) {
			const RenderTargetBase *rt = renderTargets[imageViews.size()];
			assert(rt->getFormat() == depthStencilFormat);

			if (!imageViews.size()) {
				width = rt->getWidth();
				height = rt->getHeight();
				layers = rt->getArrayLayers();
			} else {
				assert(width == rt->getWidth());
				assert(height == rt->getHeight());
				assert(layers == rt->getArrayLayers());
			}

			imageViews.push_back(rt->getImageView());
		}

		if (colorFormat != VK_FORMAT_UNDEFINED) {
			const RenderTargetBase *rt = renderTargets[imageViews.size()];
			assert(rt->getFormat() == colorFormat);

			if (!imageViews.size()) {
				width = rt->getWidth();
				height = rt->getHeight();
				layers = rt->getArrayLayers();
			} else {
				assert(width == rt->getWidth());
				assert(height == rt->getHeight());
				assert(layers == rt->getArrayLayers());
			}

			imageViews.push_back(rt->getImageView());

			if (sampleCount != VK_SAMPLE_COUNT_1_BIT) {
				const RenderTargetBase *rt = renderTargets[imageViews.size()];
				assert(rt->getFormat() == colorFormat);

				assert(width == rt->getWidth());
				assert(height == rt->getHeight());
				assert(layers == rt->getArrayLayers());

				imageViews.push_back(rt->getImageView());
			}
		}

		assert(imageViews.size() == renderTargets.size());
		assert(width > 0 && height > 0 && layers > 0);

		return vkHelpers::createFramebuffer(
			vkInstance::device,
			width, height, layers,
			imageViews, renderPass);
	}

	VkFormat getColorFormat() const { return colorFormat; }
	VkFormat getDepthStencilFormat() const { return depthStencilFormat; }
	VkSampleCountFlagBits getSampleCount() const { return sampleCount; }

private:
	VkRenderPass renderPass;

	VkFormat colorFormat, depthStencilFormat;
	VkSampleCountFlagBits sampleCount;
};

#endif // RENDERPASS_H
