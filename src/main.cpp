#include <cmath>
#include <algorithm>
#include <list>
#include <map>
#include <stdexcept>

#include "vulkan.h"
#include "core/core.h"
#include "swapchain.h"
#include "shader.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

using namespace vulkan;

using std::vector;
using std::map;
using std::exception;
using std::runtime_error;
using std::string;
using std::max;
using std::min;

static vector<const char *> getRequiredInstanceExtensions()
{
	uint32_t requiredExtentionCount;
	auto tmp = glfwGetRequiredInstanceExtensions(&requiredExtentionCount);
	return vector<const char *>(tmp, tmp + requiredExtentionCount);
}

#include "scene/rendertarget.h"

static VkPipeline createComputePipeline(VkPipelineLayout layout, VkShaderModule shaderModule, const char *name = "main")
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineCreateInfo.stage.module = shaderModule;
	computePipelineCreateInfo.stage.pName = name;
	computePipelineCreateInfo.layout = layout;

	VkPipeline computePipeline;
	assumeSuccess(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &computePipeline));
	return computePipeline;
}

static VkPhysicalDevice choosePhysicalDevice()
{
	// Get number of available physical devices
	uint32_t physicalDeviceCount = 0;
	assumeSuccess(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
	assert(physicalDeviceCount > 0);

	// Enumerate devices
	auto physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	assumeSuccess(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));
	assert(physicalDeviceCount > 0);

	auto physicalDevice = physicalDevices[0];

	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {

		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProps);

		if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			physicalDevice = physicalDevices[i];
			break;
		}
	}
	delete[] physicalDevices;

	return physicalDevice;
}

int main(int argc, char *argv[])
{
	auto appName = "cs-rast";
	auto width = 1920, height = 1080;
	auto fullscreen = false;
	GLFWwindow *win = nullptr;

	try {
		if (!glfwInit())
			throw runtime_error("glfwInit failed!");

		if (!glfwVulkanSupported())
			throw runtime_error("no vulkan support!");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		win = glfwCreateWindow(width, height, appName, fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
		if (fullscreen)
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		glfwSetKeyCallback(win, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
			if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			});


		auto enabledExtensions = getRequiredInstanceExtensions();
#ifndef NDEBUG
		enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		instanceInit(appName, enabledExtensions);

		auto physicalDevice = choosePhysicalDevice();
		deviceInit(physicalDevice, [](VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueIndex) {
			return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, queueIndex) == GLFW_TRUE;
		});

		VkSurfaceKHR surface;
		auto err = glfwCreateWindowSurface(instance, win, nullptr, &surface);
		if (err)
			throw runtime_error("glfwCreateWindowSurface failed!");

		int swapWidth = 0, swapHeight = 0;
		glfwGetFramebufferSize(win, &swapWidth, &swapHeight);
		auto swapChain = SwapChain(surface, swapWidth, swapHeight, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		ColorRenderTarget postProcessRenderTarget(VK_FORMAT_A2B10G10R10_UNORM_PACK32, width, height, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		auto postProcessDescriptorSetLayout = createDescriptorSetLayout({
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		});

		struct {
			float time;
		} postProcessPushConstantData;

		VkPushConstantRange postProcessPushConstantRange = {
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(postProcessPushConstantData)
		};

		auto postProcessPipelineLayout = createPipelineLayout({ postProcessDescriptorSetLayout }, { postProcessPushConstantRange });

		VkPipeline postProcessPipeline = createComputePipeline(postProcessPipelineLayout, loadShaderModule("data/shaders/postprocess.comp.spv"));

		int swapChainImageCount = swapChain.getImageViews().size();
		auto postProcessDescriptorPool = createDescriptorPool({
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, uint32_t(swapChainImageCount * 1) },
		}, swapChainImageCount);

		vector<VkDescriptorSet> postProcessDescriptorSets;
		postProcessDescriptorSets.reserve(swapChainImageCount);
		for (int i = 0; i < swapChainImageCount; ++i) {
			auto descriptorSet = allocateDescriptorSet(postProcessDescriptorPool, postProcessDescriptorSetLayout);
			postProcessDescriptorSets.push_back(descriptorSet);

			VkDescriptorImageInfo postProcessRenderTargetImageInfo = {};
			postProcessRenderTargetImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			postProcessRenderTargetImageInfo.imageView = postProcessRenderTarget.getImageView();

			VkWriteDescriptorSet writeDescriptorSets[1] = {};
			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].dstSet = descriptorSet;
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeDescriptorSets[0].pImageInfo = &postProcessRenderTargetImageInfo;

			vkUpdateDescriptorSets(device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
		}


		auto backBufferSemaphore = createSemaphore(),
		     presentCompleteSemaphore = createSemaphore();

		VkCommandPool commandPool = createCommandPool(graphicsQueueIndex);

		auto commandBuffers = allocateCommandBuffers(commandPool, swapChain.getImageViews().size());

		auto commandBufferFences = new VkFence[commandBuffers.size()];
		for (auto i = 0u; i < commandBuffers.size(); ++i)
			commandBufferFences[i] = createFence(VK_FENCE_CREATE_SIGNALED_BIT);

		assumeSuccess(vkQueueWaitIdle(graphicsQueue));

		while (!glfwWindowShouldClose(win)) {
			auto currentSwapImage = swapChain.aquireNextImage(backBufferSemaphore);

			assumeSuccess(vkWaitForFences(device, 1, &commandBufferFences[currentSwapImage], VK_TRUE, UINT64_MAX));
			assumeSuccess(vkResetFences(device, 1, &commandBufferFences[currentSwapImage]));

			auto commandBuffer = commandBuffers[currentSwapImage];
			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			assumeSuccess(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

			VkDescriptorSet &postProcessDescriptorSet = postProcessDescriptorSets[currentSwapImage];
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, postProcessPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, postProcessPipelineLayout, 0, 1, &postProcessDescriptorSet, 0, nullptr);

			imageBarrier(
				commandBuffer,
				postProcessRenderTarget.getImage(),
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

			
			postProcessPushConstantData.time = glfwGetTime();
			vkCmdPushConstants(commandBuffer, postProcessPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(postProcessPushConstantData), &postProcessPushConstantData);
			vkCmdDispatch(commandBuffer, (width + 15) / 16, (height + 15) / 16, 1);

			imageBarrier(
				commandBuffer,
				postProcessRenderTarget.getImage(),
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			auto swapChainImage = swapChain.getImages()[currentSwapImage];
			imageBarrier(
				commandBuffer,
				swapChainImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			blitImage(commandBuffer,
				postProcessRenderTarget.getImage(),
				swapChainImage,
				width, height,
				swapWidth, swapHeight,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 });

			imageBarrier(
				commandBuffer,
				swapChainImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, 0,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			assumeSuccess(vkEndCommandBuffer(commandBuffer));

			VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_BIND_POINT_COMPUTE;

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &backBufferSemaphore;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &presentCompleteSemaphore;
			submitInfo.pWaitDstStageMask = &waitDstStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// Submit draw command buffer
			assumeSuccess(vkQueueSubmit(graphicsQueue, 1, &submitInfo, commandBufferFences[currentSwapImage]));

			swapChain.queuePresent(currentSwapImage, &presentCompleteSemaphore, 1);

			glfwPollEvents();
		}

		assumeSuccess(vkDeviceWaitIdle(device));
		glfwDestroyWindow(win);

	} catch (const exception &e) {
		if (win != nullptr)
			glfwDestroyWindow(win);
		fprintf(stderr, "FATAL ERROR: %s\n", e.what());
	}

	glfwTerminate();
	return 0;
}
