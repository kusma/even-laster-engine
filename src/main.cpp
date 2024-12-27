#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <list>
#include <map>
#include <stdexcept>

#include "renderer/vkhelpers.h"
#include "core/core.h"
#include "core/blobbuilder.h"
#include "swapchain.h"
#include "renderer/shader.h"
#include "renderer/import-texture.h"
#include "scene/sceneimporter.h"
#include "renderer/buffer.h"
#include "renderer/scenerenderer.h"
#include "renderer/renderpass.h"
#include "renderer/pipeline.h"
#include "renderer/descriptorset.h"

#include "sync/sync.h"

const auto beatsPerMinute = 170.0f;
const auto rowsPerBeat = 8;
const auto rowRate = (beatsPerMinute / 60.0) * rowsPerBeat;

const unsigned maxConcurrentFrames = 1u;

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <bass.h>

#define FREEIMAGE_LIB
#include <FreeImage.h>

using namespace vkHelpers;

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

#include "scene/scene.h"
#include "renderer/rendertarget.h"

static VkPipeline createComputePipeline(VkPipelineLayout layout, VkShaderModule shaderModule, const char *name = "main")
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = shaderModule,
			.pName = name,
		},
		.layout = layout,
	};

	VkPipeline computePipeline;
	assumeSuccess(vkCreateComputePipelines(vkInstance::device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &computePipeline));
	return computePipeline;
}

VkPhysicalDevice choosePhysicalDevice()
{
	// Get number of available physical devices
	uint32_t physicalDeviceCount = 0;
	assumeSuccess(vkEnumeratePhysicalDevices(vkInstance::instance, &physicalDeviceCount, nullptr));
	assert(physicalDeviceCount > 0);

	// Enumerate devices
	auto physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	assumeSuccess(vkEnumeratePhysicalDevices(vkInstance::instance, &physicalDeviceCount, physicalDevices));
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

enum BlendMode {
	None,
	Additive,
	SourceOverPremult,
};

static VkPipeline createGeometrylessPipeline(VkPipelineLayout layout, const RenderPass &renderPass, const vector<VkPipelineShaderStageCreateInfo> &shaderStages, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, bool depthWrite = true, BlendMode blendMode = None)
{
	GraphicsPipelineBuilder pb;

	for (const auto shaderStage: shaderStages)
		pb.addShaderStage(shaderStage);

	pb.setIAState(topology);

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
	switch (blendMode) {
	case BlendMode::None:
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		break;
	case BlendMode::Additive:
		colorBlendAttachmentState.blendEnable = VK_TRUE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case BlendMode::SourceOverPremult:
		colorBlendAttachmentState.blendEnable = VK_TRUE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	}
	pb.addColorBlendAttachment(colorBlendAttachmentState);
	pb.setRasterizationSamples(renderPass.getSampleCount());

	pb.addViewport({}); // dummy
	pb.addScissor({}); // dummy

	pb.setDepthTest(depthWrite, depthWrite, VK_COMPARE_OP_ALWAYS);

	pb.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	pb.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);

	return pb.createPipeline(layout, renderPass.getRenderPass());
}

static VkPipeline createFullScreenQuadPipeline(VkPipelineLayout layout, const RenderPass &renderPass, VkShaderModule fragmentShader, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, bool depthWrite = true, BlendMode blendMode = None)
{
	auto shaderStages = createStageVector({
		.vertexShader = loadShaderModule("data/shaders/fullscreenquad.vert.spv"),
		.fragmentShader = fragmentShader,
	});
	return createGeometrylessPipeline(layout, renderPass, shaderStages, topology, depthWrite, blendMode);
}

static Texture3D loadFractalNoise(const std::string &filename, int width, int height, int depth)
{
	Texture3D texture(VK_FORMAT_R32G32B32A32_SFLOAT, width, height, depth, 1);

	auto size = sizeof(float) * 4 * width * height * depth;
	auto stagingBuffer = vkInstance::getStagingBuffer(size);
	void *ptr = stagingBuffer->map();

	FILE *fp = fopen(filename.c_str(), "rb");
	if (!fp)
		throw runtime_error("failed to open FBM cache");
	if (fread(ptr, 1, size, fp) != size)
		throw runtime_error("too small file!");
	fclose(fp);

	stagingBuffer->unmap();

	vkInstance::submitSetupCommands([&](VkCommandBuffer commandBuffer) {
		texture.uploadFromStagingBuffer(commandBuffer, stagingBuffer, 0);
	});

	setImageName(vkInstance::device, texture.getImage(), filename);
	return texture;
}

#include <fstream>
#include <sstream>
#include <sys/stat.h>

Texture3D importCubeFile(const std::string &filename)
{
	int size = 0;

	StagingBuffer *stagingBuffer = nullptr;
	float *ptr = nullptr;
	int colorsRead = 0;

	std::ifstream stream(filename);
	std::string line;
	while (getline(stream, line)) {
		if (line.empty() || line[0] == '#')
			continue;

		if (isalpha(line[0])) {
			auto sep = line.find(" ");
			auto verb = line.substr(0, sep);

			if (verb == "TITLE")
				continue; // ignore title

			if (verb == "LUT_3D_SIZE") {
				auto sizeString = line.substr(sep + 1);

				char *end = nullptr;
				size = strtol(sizeString.c_str(), &end, 10);
				if (end == nullptr)
					throw runtime_error("expected integer size");
				if (size < 1)
					throw runtime_error("size needs to be at least one");

				auto textureSize = sizeof(float) * 4 * size * size * size;
				stagingBuffer = vkInstance::getStagingBuffer(textureSize);
				ptr = static_cast<float *>(stagingBuffer->map());

				continue;
			}

			if (verb == "DOMAIN_MIN") {
				if (line != "DOMAIN_MIN 0.0 0.0 0.0")
					throw runtime_error("expected DOMAIN_MIN");
				continue;
			}

			if (verb == "DOMAIN_MAX") {
				if (line != "DOMAIN_MAX 1.0 1.0 1.0")
					throw runtime_error("expected DOMAIN_MAX");
				continue;
			}

			throw runtime_error("unrecognized verb");
		}

		if (isdigit(line[0])) {
			if (ptr == nullptr)
				throw runtime_error("expected size before color values");

			std::stringstream ss(line);

			float r = 0, g = 0, b = 0;

			ss >> r;
			if (ss.peek() != ' ')
				throw runtime_error("unexpected character");
			ss.ignore();

			ss >> g;
			if (ss.peek() != ' ')
				throw runtime_error("unexpected character");
			ss.ignore();

			ss >> b;

			if (!ss.eof())
				throw runtime_error("unexpected character");

			ptr[colorsRead * 4 + 0] = r;
			ptr[colorsRead * 4 + 1] = g;
			ptr[colorsRead * 4 + 2] = b;
			ptr[colorsRead * 4 + 3] = 1.0f;
			++colorsRead;
			continue;
		}

		throw runtime_error("unrecognized line");
	}

	if (!size)
		throw runtime_error("no LUT_3D_SIZE found");

	if (colorsRead != size * size * size)
		throw runtime_error("wrong amount of colors");

	Texture3D texture(VK_FORMAT_R32G32B32A32_SFLOAT, size, size, size, 1);
	stagingBuffer->unmap();

	vkInstance::submitSetupCommands([&](VkCommandBuffer commandBuffer) {
		texture.uploadFromStagingBuffer(commandBuffer, stagingBuffer, 0);
	});

	return texture;
}

std::vector<Texture3D> importColorLuts(string folder)
{
	std::vector<Texture3D> colorLuts;
	for (int i = 0; true; ++i) {
		char path[256];
		snprintf(path, sizeof(path), "%s/%04d.CUBE", folder.c_str(), i);

		struct stat st;
		if (stat(path, &st) < 0 ||
		    (st.st_mode & S_IFMT) != S_IFREG)
			break;

		auto colorLut = importCubeFile(path);
		colorLuts.push_back(colorLut);
	}

	if (colorLuts.size() == 0)
		throw runtime_error("no color-luts!");

	return colorLuts;
}

VkRect2D makeLetterbox(unsigned swapWidth, unsigned swapHeight, float monitor_aspect, float demo_aspect)
{
	float w_ratio = 1.0f,
	      h_ratio = monitor_aspect / demo_aspect;

	if (h_ratio > 1.0f) {
		/* pillar box, yo! */
		w_ratio /= h_ratio;
		h_ratio = 1.0f;
	}

	unsigned w = unsigned(std::roundf(swapWidth * w_ratio));
	unsigned h = unsigned(std::roundf(swapHeight * h_ratio));

	return {
		.offset = {int32_t(swapWidth - w) / 2, int32_t(swapHeight - h) / 2},
		.extent = {w, h},
	};
}

#ifdef _WIN32

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR lpCmdLine,
                     _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
#else
int main(int argc, char *argv[])
{
#endif

	auto appName = "CarlB - Karl Bauhaus";
	auto width = 1920, height = 1080;
	auto fullscreen = true;
	GLFWwindow *win = nullptr;

	try {
		if (!glfwInit())
			throw runtime_error("glfwInit failed!");

		if (!glfwVulkanSupported())
			throw runtime_error("no vulkan support!");

		FreeImage_Initialise(false);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		if (!fullscreen)
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

#ifdef SYNC_PLAYER
		auto monitor = glfwGetPrimaryMonitor();
#else
		GLFWmonitor* monitor = nullptr;
		fullscreen = false;
#if 0
		int monitorCount;
		GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

		if (monitorCount > 1) {
			fullscreen = true;
			monitor = monitors[monitorCount - 1];
			glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
		} else {
			fullscreen = false;
			width = 1280;
			height = 720;
		}
#endif
#endif
		win = glfwCreateWindow(width, height, appName, fullscreen ? monitor : nullptr, nullptr);
		if (fullscreen)
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		if (!BASS_Init(-1, 44100, 0, 0, 0))
			throw runtime_error("failed to init bass");

		auto stream = BASS_StreamCreateFile(false, "data/soundtrack.mp3", 0, 0, BASS_MP3_SETPOS | BASS_STREAM_PRESCAN);
		if (!stream)
			throw runtime_error("failed to open tune");

		glfwSetKeyCallback(win, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			});


		auto enabledExtensions = getRequiredInstanceExtensions();
#ifndef NDEBUG
		enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		vkInstance::instanceInit(appName, enabledExtensions);

		auto physicalDevice = choosePhysicalDevice();
		vkInstance::deviceInit(physicalDevice, [](VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueIndex) {
			return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, queueIndex) == GLFW_TRUE;
		});

		VkSurfaceKHR surface;
		auto err = glfwCreateWindowSurface(vkInstance::instance, win, nullptr, &surface);
		if (err)
			throw runtime_error("glfwCreateWindowSurface failed!");

		int swapWidth = 0, swapHeight = 0;
		glfwGetFramebufferSize(win, &swapWidth, &swapHeight);
		auto swapChain = SwapChain(surface, swapWidth, swapHeight, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		const float demo_aspect = 16.0f / 9;
		float monitor_aspect = demo_aspect;
		if (fullscreen) {
			int width_mm, height_mm;
			glfwGetMonitorPhysicalSize(monitor, &width_mm, &height_mm);
			monitor_aspect = float(width_mm) / height_mm;
		}

		VkRect2D letterbox = makeLetterbox(swapWidth, swapHeight, monitor_aspect, demo_aspect);

		vector<VkFormat> depthCandidates = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_X8_D24_UNORM_PACK32,
			VK_FORMAT_D16_UNORM,
		};

		auto sceneMSAASamples = getMaxMSAACount(vkInstance::deviceProperties);
		auto sceneFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		auto depthFormat = findBestFormat(vkInstance::physicalDevice, depthCandidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		DepthRenderTarget sceneDepthRenderTarget(depthFormat, width, height, sceneMSAASamples);
		ColorRenderTarget sceneColorMSAARenderTarget(sceneFormat, width, height, 1, sceneMSAASamples, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		ColorRenderTarget sceneColorRenderTarget(sceneFormat, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

		unsigned bloomLevels = 32 - clz(max(width, height));
		auto bloomFormat = VK_FORMAT_B10G11R11_UFLOAT_PACK32; // TODO: consider VK_FORMAT_E5B9G9R9_UFLOAT_PACK32
		ColorRenderTarget bloomRenderTarget(bloomFormat, width, height, bloomLevels, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		ColorRenderTarget postProcessRenderTarget(VK_FORMAT_A2B10G10R10_UNORM_PACK32, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		assert(sceneColorMSAARenderTarget.getFormat() == sceneColorRenderTarget.getFormat());
		RenderPass sceneRenderPass(sceneFormat, depthFormat, sceneMSAASamples);
		auto sceneFramebuffer = sceneRenderPass.createFramebuffer(
			{ &sceneDepthRenderTarget, &sceneColorMSAARenderTarget, &sceneColorRenderTarget });

		RenderPass particleRenderPass(sceneFormat);

		auto particleFramebuffer = particleRenderPass.createFramebuffer(
			{ &sceneColorRenderTarget });


		DescriptorSetBuilder backgroundDescriptorSetBuilder;
		backgroundDescriptorSetBuilder.addCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
		auto backgroundDescriptorSetLayout = backgroundDescriptorSetBuilder.createDescriptorSetLayout();

		struct {
			float time;
			float alpha;
		} backgroundPushConstantData;

		VkPushConstantRange backgroundPushConstantRange = {
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(backgroundPushConstantData)
		};

		auto backgroundPipelineLayout = createPipelineLayout(vkInstance::device, { backgroundDescriptorSetLayout }, { backgroundPushConstantRange });
		auto backgroundFragmentShader = loadShaderModule("data/shaders/background.frag.spv");
		auto backgroundPipeline = createFullScreenQuadPipeline(backgroundPipelineLayout, sceneRenderPass, backgroundFragmentShader, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, BlendMode::SourceOverPremult);

		auto backgroundDescriptorPool = backgroundDescriptorSetBuilder.createDescriptorPool(1);
		auto backgroundDescriptorSet = allocateDescriptorSet(vkInstance::device, backgroundDescriptorPool, backgroundDescriptorSetLayout);

		auto kickflipBGTexture = importTexture2D("assets/kickflip-bg.png", TextureImportFlags::NONE);
		auto kickflipTexture = importTexture2D("assets/kickflip.png", TextureImportFlags::PREMULTIPLY_ALPHA);
		VkSampler linearSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, 0.0f, false, false);
		updateCombinedImageDescriptor(vkInstance::device, backgroundDescriptorSet, 0,
		{ {
			.sampler = linearSampler,
			.imageView = kickflipBGTexture->getImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		}, {
			.sampler = linearSampler,
			.imageView = kickflipTexture->getImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		} });

		RenderPass bloomDownscaleRenderPass(bloomRenderTarget.getFormat(),
		                                    VK_FORMAT_UNDEFINED,
		                                    VK_SAMPLE_COUNT_1_BIT,
		                                    VK_ATTACHMENT_LOAD_OP_DONT_CARE);

		DescriptorSetBuilder bloomDownscaleDescriptorSetBuilder;
		bloomDownscaleDescriptorSetBuilder.addCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto bloomDownscaleDescriptorSetLayout = bloomDownscaleDescriptorSetBuilder.createDescriptorSetLayout();

		DescriptorSetBuilder bloomUpscaleDescriptorSetBuilder;
		bloomUpscaleDescriptorSetBuilder.addCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto bloomUpscaleDescriptorSetLayout = bloomUpscaleDescriptorSetBuilder.createDescriptorSetLayout();
		auto bloomDescriptorPool = bloomUpscaleDescriptorSetBuilder.createDescriptorPool(bloomLevels + bloomLevels - 1);

		const vector<VkImageView> &bloomImageViews = bloomRenderTarget.getMipImageViews();
		vector<VkFramebuffer> bloomDownscaleFramebuffers;
		vector<VkDescriptorSet> bloomDownscaleDescriptorSets;

		VkSampler bloomDownscaleInputSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, 0.0f, false, false);
		for (unsigned mipLevel = 0; mipLevel < bloomLevels; ++mipLevel) {
			auto imageView = bloomImageViews[mipLevel];

			auto mipWidth = TextureBase::mipSize(bloomRenderTarget.getWidth(), mipLevel);
			auto mipHeight = TextureBase::mipSize(bloomRenderTarget.getHeight(), mipLevel);
			auto framebuffer = createFramebuffer(vkInstance::device, mipWidth, mipHeight, 1, { imageView }, bloomDownscaleRenderPass.getRenderPass());
			bloomDownscaleFramebuffers.push_back(framebuffer);
			auto descriptorSet = allocateDescriptorSet(vkInstance::device, bloomDescriptorPool, bloomDownscaleDescriptorSetLayout);


			VkDescriptorImageInfo descriptorImageInfo = {
				.sampler = bloomDownscaleInputSampler,
				.imageView = mipLevel == 0 ?
				             sceneColorRenderTarget.getImageView() :
				             bloomImageViews[mipLevel - 1],
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			updateCombinedImageDescriptor(vkInstance::device, descriptorSet,
			                              0, { descriptorImageInfo });

			bloomDownscaleDescriptorSets.push_back(descriptorSet);
		}

		auto bloomDownscalePipelineLayout = createPipelineLayout(vkInstance::device, { bloomDownscaleDescriptorSetLayout }, {});
		auto bloomDownscaleFragmentShader = loadShaderModule("data/shaders/bloom_downscale.frag.spv");
		auto bloomDownscalePipeline = createFullScreenQuadPipeline(bloomDownscalePipelineLayout, bloomDownscaleRenderPass, bloomDownscaleFragmentShader);

		RenderPass bloomUpscaleRenderPass(bloomRenderTarget.getFormat(),
		                                  VK_FORMAT_UNDEFINED,
		                                  VK_SAMPLE_COUNT_1_BIT,
		                                  VK_ATTACHMENT_LOAD_OP_LOAD,
		                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		vector<VkFramebuffer> bloomUpscaleFramebuffers;
		vector<VkDescriptorSet> bloomUpscaleDescriptorSets;

		VkSampler bloomUpscaleInputSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, 0.0f, false, false);
		for (unsigned mipLevel = 0; mipLevel < bloomLevels - 1; ++mipLevel) {
			auto mipWidth = TextureBase::mipSize(bloomRenderTarget.getWidth(), mipLevel);
			auto mipHeight = TextureBase::mipSize(bloomRenderTarget.getHeight(), mipLevel);
			auto framebuffer = createFramebuffer(vkInstance::device, mipWidth, mipHeight, 1, { bloomImageViews[mipLevel] }, bloomUpscaleRenderPass.getRenderPass());
			bloomUpscaleFramebuffers.push_back(framebuffer);

			auto descriptorSet = allocateDescriptorSet(vkInstance::device, bloomDescriptorPool, bloomUpscaleDescriptorSetLayout);

			VkDescriptorImageInfo descriptorImageInfo = {
				.sampler = bloomUpscaleInputSampler,
				.imageView = bloomImageViews[mipLevel + 1],
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			updateCombinedImageDescriptor(vkInstance::device, descriptorSet,
			                              0, { descriptorImageInfo });

			bloomUpscaleDescriptorSets.push_back(descriptorSet);
		}

		auto bloomUpscalePipelineLayout = createPipelineLayout(vkInstance::device, { bloomUpscaleDescriptorSetLayout }, {});
		auto bloomUpscaleFragmentShader = loadShaderModule("data/shaders/bloom_upscale.frag.spv");
		auto bloomUpscalePipeline = createFullScreenQuadPipeline(bloomUpscalePipelineLayout, bloomUpscaleRenderPass, bloomUpscaleFragmentShader, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, BlendMode::Additive);

		// particle effects

		struct {
			glm::mat4 modelViewProjectionMatrix;
			glm::vec2 offsets;
			float time;
		} particleUniforms;
		auto particleUniformBuffer = new UniformBuffer(sizeof(particleUniforms));

		// smoke effect

		struct {
			glm::vec2 offset;
			glm::vec2 scale;
			float time;
			int logoImage;
			float logoAmount;
		} smokeUniforms;
		auto smokeUniformBuffer = new UniformBuffer(sizeof(smokeUniforms));

		DescriptorSetBuilder smokeDescriptorSetBuilder;
		smokeDescriptorSetBuilder.addUniformBuffer(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT);
		smokeDescriptorSetBuilder.addUniformBuffer(1, VK_SHADER_STAGE_VERTEX_BIT);
		smokeDescriptorSetBuilder.addCombinedImageSampler(2, VK_SHADER_STAGE_VERTEX_BIT);
		smokeDescriptorSetBuilder.addCombinedImageSampler(3, VK_SHADER_STAGE_VERTEX_BIT);
		auto smokeDescriptorSetLayout = smokeDescriptorSetBuilder.createDescriptorSetLayout();

		auto smokePipelineLayout = createPipelineLayout(vkInstance::device, { smokeDescriptorSetLayout }, {});
		auto smokeShaderStages = createStageVector({
			.vertexShader = loadShaderModule("data/shaders/smoke.vert.spv"),
			.geometryShader = loadShaderModule("data/shaders/particle.geom.spv"),
			.fragmentShader = loadShaderModule("data/shaders/smoke.frag.spv"),
		});
		auto smokePipeline = createGeometrylessPipeline(smokePipelineLayout, particleRenderPass, smokeShaderStages, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, false, BlendMode::Additive);

		auto smokeDescriptorPool = smokeDescriptorSetBuilder.createDescriptorPool(1);

		auto smokeDescriptorSet = allocateDescriptorSet(vkInstance::device, smokeDescriptorPool, smokeDescriptorSetLayout);

		Texture3D fractalNoise = loadFractalNoise("data/fbm.raw", 64, 64, 64);
		VkSampler fractalNoiseSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, 0.0f, true, false);

		auto smokeText = importTexture2DArray("assets/smoke-text", TextureImportFlags::NONE);

		{
			writeUniformBufferDescriptor(vkInstance::device, smokeDescriptorSet,
			                             0, { particleUniformBuffer->getDescriptorBufferInfo() });
			writeUniformBufferDescriptor(vkInstance::device, smokeDescriptorSet,
			                             1, { smokeUniformBuffer->getDescriptorBufferInfo() });
			updateCombinedImageDescriptor(vkInstance::device, smokeDescriptorSet,
			                              2, { { fractalNoiseSampler, fractalNoise.getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } });
			updateCombinedImageDescriptor(vkInstance::device, smokeDescriptorSet,
			                              3, { { linearSampler, smokeText->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } });
		}

		// bartikkel effect

		struct {
			glm::ivec4 xaxis;
			glm::ivec4 yaxis;
			glm::ivec4 zaxis;
			glm::ivec4 zpos;
			glm::mat4 modelViewMatrix;
		} bartikkelUniforms;
		auto bartikkelUniformBuffer = new UniformBuffer(sizeof(bartikkelUniforms));

		DescriptorSetBuilder bartikkelDescriptorSetBuilder;
		bartikkelDescriptorSetBuilder.addUniformBuffer(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT);
		bartikkelDescriptorSetBuilder.addUniformBuffer(1, VK_SHADER_STAGE_VERTEX_BIT);
		bartikkelDescriptorSetBuilder.addCombinedImageSampler(2, VK_SHADER_STAGE_FRAGMENT_BIT);
		auto bartikkelDescriptorSetLayout = bartikkelDescriptorSetBuilder.createDescriptorSetLayout();

		auto bartikkelPipelineLayout = createPipelineLayout(vkInstance::device, { bartikkelDescriptorSetLayout }, {});
		auto bartikkelShaderStages = createStageVector({
			.vertexShader = loadShaderModule("data/shaders/bartikkel.vert.spv"),
			.geometryShader = loadShaderModule("data/shaders/particle.geom.spv"),
			.fragmentShader = loadShaderModule("data/shaders/bartikkel.frag.spv"),
		});
		auto bartikkelPipeline = createGeometrylessPipeline(bartikkelPipelineLayout, particleRenderPass, bartikkelShaderStages, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, false, BlendMode::SourceOverPremult);

		auto bartikkelDescriptorPool = bartikkelDescriptorSetBuilder.createDescriptorPool(1);

		auto bartikkelDescriptorSet = allocateDescriptorSet(vkInstance::device, bartikkelDescriptorPool, bartikkelDescriptorSetLayout);

		auto bartikkelTexture = importTexture2D("assets/bartikkel.png", TextureImportFlags::PREMULTIPLY_ALPHA | TextureImportFlags::GENERATE_MIPMAPS);
		VkSampler bartikkelSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, FLT_MAX, true, false);

		{
			vector<VkDescriptorImageInfo> descriptorImageInfos = {
				{ bartikkelSampler, bartikkelTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
			};

			writeUniformBufferDescriptor(vkInstance::device, bartikkelDescriptorSet,
			                             0, { particleUniformBuffer->getDescriptorBufferInfo() });
			writeUniformBufferDescriptor(vkInstance::device, bartikkelDescriptorSet,
			                             1, { bartikkelUniformBuffer->getDescriptorBufferInfo() });
			updateCombinedImageDescriptor(vkInstance::device, bartikkelDescriptorSet,
			                              2, descriptorImageInfos);
		}



		vector<Scene *> scenes;
		for (int i = 0; true; ++i) {
			char path[256];
			snprintf(path, sizeof(path), "assets/scenes/%04d.dae", i);

			struct stat st;
			if (stat(path, &st) < 0 ||
			    (st.st_mode & S_IFMT) != S_IFREG)
				break;

			scenes.push_back(SceneImporter::import(path));
		}

		vector<SceneRenderer*> sceneRenderers;
		for (auto scene : scenes)
			sceneRenderers.push_back(new SceneRenderer(scene, sceneRenderPass));

		auto planes = importTexture2DArray("assets/planes", TextureImportFlags::NONE);
		auto overlays = importTexture2DArray("assets/overlays", TextureImportFlags::PREMULTIPLY_ALPHA);
		auto cubeTexture = importTextureCube("assets/cubemap.hdr", TextureImportFlags::GENERATE_MIPMAPS);
		auto colorLuts = importColorLuts("assets/luts");

		VkSampler textureSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, float(planes->getMipLevels()), false, false);
		for (SceneRenderer *sceneRenderer : sceneRenderers) {
			vector<VkDescriptorImageInfo> descriptorImageInfos = {
				cubeTexture->getDescriptorImageInfo(textureSampler)
			};

			auto descriptorSets = sceneRenderer->getDescriptorSets();
			for (const auto kv : descriptorSets) {
				updateCombinedImageDescriptor(vkInstance::device,
				                              kv.second,
				                              1, descriptorImageInfos);
			}
		}

		auto arrayTextureSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, 0.0f, false, false);
		auto colorLutSampler = createSampler(vkInstance::device, vkInstance::enabledFeatures, vkInstance::deviceProperties, 0.0f, false, false);

		DescriptorSetBuilder postProcessDescriptorSetBuilder;
		postProcessDescriptorSetBuilder.addStorageImage(0, VK_SHADER_STAGE_COMPUTE_BIT);
		for (int i = 1; i < 6; ++i)
			postProcessDescriptorSetBuilder.addCombinedImageSampler(i, VK_SHADER_STAGE_COMPUTE_BIT);
		auto postProcessDescriptorSetLayout = postProcessDescriptorSetBuilder.createDescriptorSetLayout();
		struct {
			uint32_t overlayIndex;
			uint32_t frameSeed;
			float bloomAmount;
			float overlayAlpha;
			float fade;
			float flash;
			float kaleidoCount;
			float gradeBlend;
			float gradeAmount;
		} postProcessPushConstantData;

		VkPushConstantRange postProcessPushConstantRange = {
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(postProcessPushConstantData)
		};

		auto postProcessPipelineLayout = createPipelineLayout(vkInstance::device, { postProcessDescriptorSetLayout }, { postProcessPushConstantRange });

		VkPipeline postProcessPipeline = createComputePipeline(postProcessPipelineLayout, loadShaderModule("data/shaders/postprocess.comp.spv"));

		int swapChainImageCount = swapChain.getImageViews().size();
		auto postProcessDescriptorPool = postProcessDescriptorSetBuilder.createDescriptorPool(swapChainImageCount);

		vector<VkDescriptorSet> postProcessDescriptorSets;
		postProcessDescriptorSets.reserve(swapChainImageCount);
		for (int i = 0; i < swapChainImageCount; ++i) {
			auto descriptorSet = allocateDescriptorSet(vkInstance::device, postProcessDescriptorPool, postProcessDescriptorSetLayout);
			postProcessDescriptorSets.push_back(descriptorSet);

			VkDescriptorImageInfo postProcessRenderTargetImageInfo = {
				.imageView = postProcessRenderTarget.getImageView(),
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			};

			vector<VkDescriptorImageInfo> descriptorImageInfos = {
				{ arrayTextureSampler, sceneColorRenderTarget.getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ arrayTextureSampler, bloomRenderTarget.getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ arrayTextureSampler, overlays->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
			};

			updateStorageImageDescriptor(vkInstance::device, descriptorSet,
			                             0, { postProcessRenderTargetImageInfo });
			updateCombinedImageDescriptor(vkInstance::device, descriptorSet,
			                              1, descriptorImageInfos);
		}


		auto presentCompleteSemaphore = createSemaphore(vkInstance::device);

		VkCommandPool commandPool = createCommandPool(vkInstance::device, vkInstance::graphicsQueueIndex);

		auto commandBuffers = allocateCommandBuffers(vkInstance::device, commandPool, maxConcurrentFrames);

		auto commandBufferFences = new VkFence[maxConcurrentFrames];
		auto commandBufferSemaphores = new VkSemaphore[maxConcurrentFrames];
		for (auto i = 0u; i < maxConcurrentFrames; ++i) {
			commandBufferFences[i] = createFence(vkInstance::device, VK_FENCE_CREATE_SIGNALED_BIT);
			commandBufferSemaphores[i] = createSemaphore(vkInstance::device);
		}

		auto rocket = sync_create_device("data/sync");
		if (!rocket)
			throw runtime_error("sync_create_device() failed: out of memory?");

#ifndef SYNC_PLAYER
		if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
			throw runtime_error("failed to connect to host");
#endif

		auto sceneIndexTrack = sync_get_track(rocket, "scene.index");

		auto clearRTrack = sync_get_track(rocket, "background:clear.r");
		auto clearGTrack = sync_get_track(rocket, "background:clear.g");
		auto clearBTrack = sync_get_track(rocket, "background:clear.b");

		auto cameraFOVTrack = sync_get_track(rocket, "camera:fov");
		auto cameraRotYTrack = sync_get_track(rocket, "camera:rot.y");
		auto cameraDistTrack = sync_get_track(rocket, "camera:dist");
		auto cameraRollTrack = sync_get_track(rocket, "camera:roll");
		auto cameraUpTrack = sync_get_track(rocket, "camera:up");
		auto cameraTargetXTrack = sync_get_track(rocket, "camera:target.x");
		auto cameraTargetYTrack = sync_get_track(rocket, "camera:target.y");
		auto cameraTargetZTrack = sync_get_track(rocket, "camera:target.z");

		auto bloomAmountTrack = sync_get_track(rocket, "postprocess:bloom.amount");
		auto kaleidoTrack = sync_get_track(rocket, "postprocess:kaleidoscope");

		auto gradeIndex1Track = sync_get_track(rocket, "postprocess:grade.index1");
		auto gradeIndex2Track = sync_get_track(rocket, "postprocess:grade.index2");
		auto gradeBlendTrack = sync_get_track(rocket, "postprocess:grade.blend");
		auto gradeAmountTrack = sync_get_track(rocket, "postprocess:grade.amount");

		auto overlayIndexTrack = sync_get_track(rocket, "overlay.index");
		auto overlayAlphaTrack = sync_get_track(rocket, "overlay.alpha");
		auto fadeTrack = sync_get_track(rocket, "fade");
		auto flashTrack = sync_get_track(rocket, "flash");
		auto pulseAmountTrack = sync_get_track(rocket, "pulse.amount");
		auto pulseSpeedTrack = sync_get_track(rocket, "pulse.speed");

		auto wavePlaneOffsetXTrack = sync_get_track(rocket, "waveplane:offset.x");
		auto wavePlaneOffsetYTrack = sync_get_track(rocket, "waveplane:offset.y");
		auto wavePlaneScaleXTrack = sync_get_track(rocket, "waveplane:scale.x");
		auto wavePlaneScaleYTrack = sync_get_track(rocket, "waveplane:scale.y");
		auto wavePlaneTimeTrack = sync_get_track(rocket, "waveplane:time");
		auto logoImageTrack = sync_get_track(rocket, "waveplane:image");
		auto logoAmoutTrack = sync_get_track(rocket, "waveplane:logo");

		auto flipTrack = sync_get_track(rocket, "kickflip:flip");
		auto backgroundAlphaTrack = sync_get_track(rocket, "kickflip:bg-alpha");

		// wait for all pending setup-work to finish
		vkInstance::finishSetup();

		BASS_Start();
		BASS_ChannelPlay(stream, false);

		unsigned frameIndex = 0;
		while (!glfwWindowShouldClose(win)) {
			auto pos = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
			auto time = BASS_ChannelBytes2Seconds(stream, pos);
			auto row = time * rowRate;

#ifndef SYNC_PLAYER
			static sync_cb bassCallbacks = {
				// pause
				[](void *d, int flag) {
					HSTREAM h = *((HSTREAM *)d);
					if (flag)
						BASS_ChannelPause(h);
					else
						BASS_ChannelPlay(h, false);
				},
				// set row
				[](void *d, int row) {
					HSTREAM h = *((HSTREAM *)d);
					QWORD pos = BASS_ChannelSeconds2Bytes(h, (row + 0.01) / rowRate);
					BASS_ChannelSetPosition(h, pos, BASS_POS_BYTE);
				},
				// is playing
				[](void *d) -> int {
					HSTREAM h = *((HSTREAM *)d);
					return BASS_ChannelIsActive(h) == BASS_ACTIVE_PLAYING;
				},
			};

			if (sync_update(rocket, int(floor(row)), &bassCallbacks, (void *)&stream))
				sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

			assert(frameIndex < maxConcurrentFrames);
			assumeSuccess(vkWaitForFences(vkInstance::device, 1, &commandBufferFences[frameIndex], VK_TRUE, UINT64_MAX));
			assumeSuccess(vkResetFences(vkInstance::device, 1, &commandBufferFences[frameIndex]));
			auto currentSwapImage = swapChain.aquireNextImage(commandBufferSemaphores[frameIndex]);

			auto commandBuffer = commandBuffers[frameIndex];

			VkCommandBufferBeginInfo commandBufferBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			};

			assumeSuccess(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

			auto th = sync_get_val(cameraRotYTrack, row) * (M_PI / 180);
			auto dist = sync_get_val(cameraDistTrack, row);
			auto roll = sync_get_val(cameraRollTrack, row) * (M_PI / 180);
			auto flip = sync_get_val(flipTrack, row) * (M_PI / 180);

			auto cameraTargetX = sync_get_val(cameraTargetXTrack, row);
			auto cameraTargetY = sync_get_val(cameraTargetYTrack, row);
			auto cameraTargetZ = sync_get_val(cameraTargetZTrack, row);

			auto targetPosition = glm::vec3(
				float(cameraTargetX),
				float(cameraTargetY),
				float(cameraTargetZ));
			auto viewPosition = glm::vec3(
				cameraTargetX + sin(th) * dist,
				cameraTargetY + sync_get_val(cameraUpTrack, row),
				cameraTargetZ + cos(th) * dist);
			auto lookAt = glm::lookAt(viewPosition, targetPosition, glm::vec3(0, 1, 0));
			auto viewMatrix = glm::rotate(glm::mat4(1), float(roll), glm::vec3(0, 0, 1)) * lookAt * glm::rotate(glm::mat4(1), float(flip), glm::vec3(1, 0, 0));

			auto fov = sync_get_val(cameraFOVTrack, row);
			auto aspect = float(width) / height;
			auto znear = 0.01f;
			auto zfar = 500.0f;
			auto projectionMatrix = glm::perspective(float(fov * M_PI / 180), aspect, znear, zfar);

			int sceneIndex = int(sync_get_val(sceneIndexTrack, row));
			if (sceneIndex >= 0) {
				sceneIndex %= sceneRenderers.size();
				SceneRenderer *sceneRenderer = sceneRenderers[sceneIndex];

				VkClearValue clearValues[] = { {
						.depthStencil = { 1.0f, 0 }
					}, {
						.color = {
							float(sync_get_val(clearRTrack, row)),
							float(sync_get_val(clearGTrack, row)),
							float(sync_get_val(clearBTrack, row)),
							1.0f
						},
					}, {
						.color = { }, // not used
					},
				};

				VkRenderPassBeginInfo sceneRenderPassBegin = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = sceneRenderPass.getRenderPass(),
					.framebuffer = sceneFramebuffer,
					.renderArea = {
						.extent = {uint32_t(width), uint32_t(height)},
					},
					.clearValueCount = ARRAY_SIZE(clearValues),
					.pClearValues = clearValues,
				};

				vkCmdBeginRenderPass(commandBuffer, &sceneRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

				setViewport(commandBuffer, 0, 0, float(width), float(height));
				setScissor(commandBuffer, 0, 0, width, height);

				auto backgroundAlpha = sync_get_val(backgroundAlphaTrack, row);
				if (backgroundAlpha > 0) {
					backgroundPushConstantData.time = row;
					backgroundPushConstantData.alpha = backgroundAlpha;
					vkCmdPushConstants(commandBuffer, backgroundPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(backgroundPushConstantData), &backgroundPushConstantData);
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backgroundPipeline);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backgroundPipelineLayout, 0, 1, &backgroundDescriptorSet, 0, nullptr);
					vkCmdDraw(commandBuffer, 3, 1, 0, 0);
				}

				sceneRenderer->draw(commandBuffer, viewMatrix, projectionMatrix);
				vkCmdEndRenderPass(commandBuffer);
			} else {
				auto modelMatrix = glm::mat4(1);
				auto modelViewMatrix = viewMatrix * modelMatrix;
				auto modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
				auto a = projectionMatrix[0].x;
				auto b = projectionMatrix[1].y;

				particleUniforms.modelViewProjectionMatrix = modelViewProjectionMatrix;
				particleUniforms.offsets = glm::vec2(a, b);

				bufferBarrier(commandBuffer,
					particleUniformBuffer->getBuffer(),
					VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_UNIFORM_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT);

				vkCmdUpdateBuffer(commandBuffer, particleUniformBuffer->getBuffer(), 0, sizeof(particleUniforms), &particleUniforms);

				bufferBarrier(commandBuffer,
					particleUniformBuffer->getBuffer(),
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_UNIFORM_READ_BIT);

				VkClearValue clearValue = {
					.color = {
						float(sync_get_val(clearRTrack, row)),
						float(sync_get_val(clearGTrack, row)),
						float(sync_get_val(clearBTrack, row)),
						1.0f
					}
				};

				VkRenderPassBeginInfo particleRenderPassBegin = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = particleRenderPass.getRenderPass(),
					.framebuffer = particleFramebuffer,
					.renderArea = {
						.extent = {uint32_t(width), uint32_t(height)},
					},
					.clearValueCount = 1,
					.pClearValues = &clearValue,
				};

				if (sceneIndex == -1) {
					smokeUniforms.offset = glm::vec2(sync_get_val(wavePlaneOffsetXTrack, row),
													sync_get_val(wavePlaneOffsetYTrack, row));
					smokeUniforms.scale = glm::vec2(sync_get_val(wavePlaneScaleXTrack, row),
													sync_get_val(wavePlaneScaleYTrack, row));
					smokeUniforms.time = float(sync_get_val(wavePlaneTimeTrack, row));
					smokeUniforms.logoImage = int(sync_get_val(logoImageTrack, row));
					smokeUniforms.logoAmount = float(sync_get_val(logoAmoutTrack, row));

					bufferBarrier(commandBuffer,
						smokeUniformBuffer->getBuffer(),
						VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_UNIFORM_READ_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT);

					vkCmdUpdateBuffer(commandBuffer, smokeUniformBuffer->getBuffer(), 0, sizeof(smokeUniforms), &smokeUniforms);

					bufferBarrier(commandBuffer,
						smokeUniformBuffer->getBuffer(),
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT,
						VK_ACCESS_UNIFORM_READ_BIT);

					vkCmdBeginRenderPass(commandBuffer, &particleRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

					setViewport(commandBuffer, 0, 0, float(width), float(height));
					setScissor(commandBuffer, 0, 0, width, height);

					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, smokePipeline);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, smokePipelineLayout, 0, 1, &smokeDescriptorSet, 0, nullptr);
					int size = 3 << 8;
					vkCmdDraw(commandBuffer, size, size, 0, 0);
				} else {

					const int AXIS_BITS = 6;
					glm::vec3 xaxis = modelViewMatrix[0];
					glm::vec3 yaxis = modelViewMatrix[1];
					glm::vec3 zaxis = modelViewMatrix[2];
					glm::vec3 zpos = -modelViewMatrix[3];

					// make sure each axis has positive z
					if (xaxis.z < 0)
						xaxis = -xaxis;
					if (yaxis.z < 0)
						yaxis = -yaxis;
					if (zaxis.z < 0)
						zaxis = -zaxis;

					if (xaxis.z > yaxis.z)
						std::swap(xaxis, yaxis);

					if (xaxis.z > zaxis.z)
						std::swap(xaxis, zaxis);

					if (yaxis.z > zaxis.z)
						std::swap(yaxis, zaxis);

					// center the grid
					zpos -= xaxis * float(1 << (AXIS_BITS - 1));
					zpos -= yaxis * float(1 << (AXIS_BITS - 1));
					zpos -= zaxis * float(1 << (AXIS_BITS - 1));

					auto gridMatrix = glm::mat4(glm::vec4(xaxis, 0),
					                            glm::vec4(yaxis, 0),
					                            glm::vec4(zaxis, 0),
					                            glm::vec4(zpos, 0));

					gridMatrix = glm::inverse(modelViewMatrix) * gridMatrix;

					bartikkelUniforms.xaxis = glm::ivec4(roundf(gridMatrix[0].x), roundf(gridMatrix[0].y), roundf(gridMatrix[0].z), 0);
					bartikkelUniforms.yaxis = glm::ivec4(roundf(gridMatrix[1].x), roundf(gridMatrix[1].y), roundf(gridMatrix[1].z), 0);
					bartikkelUniforms.zaxis = glm::ivec4(roundf(gridMatrix[2].x), roundf(gridMatrix[2].y), roundf(gridMatrix[2].z), 0);
					bartikkelUniforms.zpos = glm::ivec4(roundf(gridMatrix[3].x), roundf(gridMatrix[3].y), roundf(gridMatrix[3].z), 0);
					bartikkelUniforms.modelViewMatrix = modelViewMatrix;

					bufferBarrier(commandBuffer,
						bartikkelUniformBuffer->getBuffer(),
						VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_UNIFORM_READ_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT);

					vkCmdUpdateBuffer(commandBuffer, bartikkelUniformBuffer->getBuffer(), 0, sizeof(bartikkelUniforms), &bartikkelUniforms);

					bufferBarrier(commandBuffer,
						bartikkelUniformBuffer->getBuffer(),
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT,
						VK_ACCESS_UNIFORM_READ_BIT);

					vkCmdBeginRenderPass(commandBuffer, &particleRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

					setViewport(commandBuffer, 0, 0, float(width), float(height));
					setScissor(commandBuffer, 0, 0, width, height);

					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bartikkelPipeline);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bartikkelPipelineLayout, 0, 1, &bartikkelDescriptorSet, 0, nullptr);
					int size = 1 << (3 * AXIS_BITS);
					vkCmdDraw(commandBuffer, size, 1, 0, 0);
				}

				vkCmdEndRenderPass(commandBuffer);
			}

			imageBarrier(
				commandBuffer,
				sceneColorRenderTarget.getImage(),
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			for (unsigned i = 0; i < bloomLevels; ++i) {
				auto levelWidth = TextureBase::mipSize(bloomRenderTarget.getWidth(), i);
				auto levelHeight = TextureBase::mipSize(bloomRenderTarget.getHeight(), i);
				VkRenderPassBeginInfo bloomRenderPassBegin = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = bloomDownscaleRenderPass.getRenderPass(),
					.framebuffer = bloomDownscaleFramebuffers[i],
					.renderArea = {
						.extent = {levelWidth, levelHeight},
					},
				};

				vkCmdBeginRenderPass(commandBuffer, &bloomRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

				setViewport(commandBuffer, 0, 0, float(levelWidth), float(levelHeight));
				setScissor(commandBuffer, 0, 0, levelWidth, levelHeight);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomDownscalePipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomDownscalePipelineLayout, 0, 1, &bloomDownscaleDescriptorSets[i], 0, nullptr);
				vkCmdDraw(commandBuffer, 3, 1, 0, 0);

				vkCmdEndRenderPass(commandBuffer);
			}

			VkImageSubresourceRange subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = bloomLevels - 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};

			imageBarrier(
				commandBuffer,
				bloomRenderTarget.getImage(),
				subresourceRange,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			for (int i = bloomLevels - 2; i >= 0; --i) {
				auto levelWidth = TextureBase::mipSize(bloomRenderTarget.getWidth(), i);
				auto levelHeight = TextureBase::mipSize(bloomRenderTarget.getHeight(), i);
				VkRenderPassBeginInfo bloomRenderPassBegin = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = bloomUpscaleRenderPass.getRenderPass(),
					.framebuffer = bloomUpscaleFramebuffers[i],
					.renderArea = {
						.extent = {levelWidth, levelHeight},
					},
				};

				vkCmdBeginRenderPass(commandBuffer, &bloomRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

				setViewport(commandBuffer, 0, 0, float(levelWidth), float(levelHeight));
				setScissor(commandBuffer, 0, 0, levelWidth, levelHeight);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomUpscalePipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomUpscalePipelineLayout, 0, 1, &bloomUpscaleDescriptorSets[i], 0, nullptr);
				vkCmdDraw(commandBuffer, 3, 1, 0, 0);

				vkCmdEndRenderPass(commandBuffer);
			}

			VkDescriptorSet &postProcessDescriptorSet = postProcessDescriptorSets[currentSwapImage];
			{
				auto gradeIndex1 = max(0, min(int(sync_get_val(gradeIndex1Track, row)), int(colorLuts.size() - 1)));
				auto gradeIndex2 = max(0, min(int(sync_get_val(gradeIndex2Track, row)), int(colorLuts.size() - 1)));

				vector<VkDescriptorImageInfo> descriptorImageInfos = {
					{ colorLutSampler, colorLuts[gradeIndex1].getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					{ colorLutSampler, colorLuts[gradeIndex2].getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
				};

				updateCombinedImageDescriptor(vkInstance::device,
				                              postProcessDescriptorSet,
				                              4, descriptorImageInfos);
			}

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, postProcessPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, postProcessPipelineLayout, 0, 1, &postProcessDescriptorSet, 0, nullptr);

			imageBarrier(
				commandBuffer,
				postProcessRenderTarget.getImage(),
				VK_IMAGE_ASPECT_COLOR_BIT, // ?? does this really do anything?!
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL);

			imageBarrier(
				commandBuffer,
				bloomRenderTarget.getImage(),
				subresourceRange,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			auto fade = sync_get_val(fadeTrack, row);
			auto pulseAmount = sync_get_val(pulseAmountTrack, row);
			auto pulseSpeed = sync_get_val(pulseSpeedTrack, row);
			auto pulse = cos(row * pulseSpeed * (M_PI / rowsPerBeat));
			// fade = max(0.0, fade - pulseAmount + pulse * pulseAmount);

			auto bloomAmount = float(sync_get_val(bloomAmountTrack, row));
			bloomAmount = max(0.0, bloomAmount - pulseAmount + pulse * pulseAmount);

			postProcessPushConstantData.overlayIndex = uint32_t(sync_get_val(overlayIndexTrack, row));
			postProcessPushConstantData.frameSeed = rand();
			postProcessPushConstantData.bloomAmount = bloomAmount;
			postProcessPushConstantData.overlayAlpha = float(sync_get_val(overlayAlphaTrack, row));
			postProcessPushConstantData.fade = float(fade);
			postProcessPushConstantData.flash = float(sync_get_val(flashTrack, row));
			postProcessPushConstantData.kaleidoCount = float(sync_get_val(kaleidoTrack, row));
			postProcessPushConstantData.gradeBlend = float(sync_get_val(gradeBlendTrack, row));
			postProcessPushConstantData.gradeAmount = float(sync_get_val(gradeAmountTrack, row));

			vkCmdPushConstants(commandBuffer, postProcessPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(postProcessPushConstantData), &postProcessPushConstantData);
			vkCmdDispatch(commandBuffer, (width + 15) / 16, (height + 15) / 16, 1);

			imageBarrier(
				commandBuffer,
				postProcessRenderTarget.getImage(),
				VK_IMAGE_ASPECT_COLOR_BIT, // ??
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			auto swapChainImage = swapChain.getImages()[currentSwapImage];
			imageBarrier(
				commandBuffer,
				swapChainImage,
				VK_IMAGE_ASPECT_COLOR_BIT, // ??
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			if (letterbox.offset.x != 0 ||
			    letterbox.offset.y != 0 ||
				int(letterbox.extent.width) != width ||
				int(letterbox.extent.height) != height) {
				// clear yo
				VkClearColorValue color = { 0 };
				VkImageSubresourceRange range = {
					VK_IMAGE_ASPECT_COLOR_BIT,
					0, 1, 0, 1
				};
				vkCmdClearColorImage(
					commandBuffer,
					swapChainImage,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					&color,
					1,
					&range);

				// clear and following blit could happen on different HW queues, so we need a
				// barrier here..
				imageBarrier(
					commandBuffer,
					swapChainImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			}

			blitImage(commandBuffer,
				postProcessRenderTarget.getImage(),
				swapChainImage,
				width, height,
				letterbox,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 });

			imageBarrier(
				commandBuffer,
				swapChainImage,
				VK_IMAGE_ASPECT_COLOR_BIT, // ??
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, 0,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			assumeSuccess(vkEndCommandBuffer(commandBuffer));

			VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_BIND_POINT_COMPUTE;

			VkSubmitInfo submitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &commandBufferSemaphores[frameIndex],
				.pWaitDstStageMask = &waitDstStageMask,
				.commandBufferCount = 1,
				.pCommandBuffers = &commandBuffer,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &presentCompleteSemaphore,
			};

			// Submit draw command buffer
			assumeSuccess(vkQueueSubmit(vkInstance::graphicsQueue, 1, &submitInfo, commandBufferFences[frameIndex]));

			swapChain.queuePresent(currentSwapImage, &presentCompleteSemaphore, 1);

			frameIndex++;
			if (frameIndex == maxConcurrentFrames)
				frameIndex = 0;

			glfwPollEvents();

#ifdef SYNC_PLAYER
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_STOPPED)
				break;
#endif
		}

#ifndef SYNC_PLAYER
		sync_save_tracks(rocket);
#endif
		sync_destroy_device(rocket);

		assumeSuccess(vkDeviceWaitIdle(vkInstance::device));

		for (auto sr : sceneRenderers)
			delete sr;
		sceneRenderers.clear();

		glfwDestroyWindow(win);

	} catch (const exception &e) {
		if (win != nullptr)
			glfwDestroyWindow(win);

#ifdef _WIN32
		MessageBox(nullptr, e.what(), nullptr, MB_OK);
#else
		fprintf(stderr, "FATAL ERROR: %s\n", e.what());
#endif
	}

	glfwTerminate();
	return 0;
}
