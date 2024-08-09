#ifndef PIPELINE_H
#define PIPELINE_H

#include "renderpass.h"
#include <vector>
#include <set>

#include "vkhelpers.h"

class GraphicsPipelineBuilder {
public:
	GraphicsPipelineBuilder();

	VkPipeline createPipeline(VkPipelineLayout layout, VkRenderPass renderPass);

	void addShaderStage(const VkPipelineShaderStageCreateInfo &stage)
	{
		assert(stage.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
		stages.push_back(stage);
	}

	void addVertexInputBinding(const VkVertexInputBindingDescription &binding)
	{
		vertexInputBindingDescriptions.push_back(binding);
	}

	void addVertexInputBinding(unsigned binding, unsigned stride,
	                           VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX)
	{
		vertexInputBindingDescriptions.push_back({
			.binding = binding,
			.stride = stride,
			.inputRate = inputRate,
		});
	}

	void addVertexInputAttribute(const VkVertexInputAttributeDescription &attrib)
	{
		vertexAttributeDescriptions.push_back(attrib);
	}

	void addVertexInputAttribute(unsigned location, unsigned binding,
	                             VkFormat format, unsigned offset)
	{
		vertexAttributeDescriptions.push_back({
			.location = location,
			.binding = binding,
			.format = format,
			.offset = offset,
		});
	}

	void setIAState(VkPrimitiveTopology topology,
	                bool primitiveRestart = false)
	{
		inputAssemblyStateCreateInfo.topology = topology;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestart;
	}

	void addColorBlendAttachment(const VkPipelineColorBlendAttachmentState &state)
	{
		colorBlendAttachmentStates.push_back(state);
	}

	void setRasterizationSamples(VkSampleCountFlagBits sampleCount)
	{
		multisampleStateCreateInfo.rasterizationSamples = sampleCount;
	}

	void addViewport(VkViewport viewport)
	{
		viewports.push_back(viewport);
	}

	void addScissor(VkRect2D scissor)
	{
		scissors.push_back(scissor);
	}

	void addDynamicState(VkDynamicState dynamicState)
	{
		dynamicStateEnables.insert(dynamicState);
	}

	void setDepthTest(bool depthTest = false, bool depthWrite = false,
	                  VkCompareOp depthCompare = VK_COMPARE_OP_ALWAYS)
	{
		depthStencilStateCreateInfo.depthTestEnable = depthTest;
		depthStencilStateCreateInfo.depthWriteEnable = depthWrite;
		depthStencilStateCreateInfo.depthCompareOp = depthCompare;
	}

	void setCullMode(VkCullModeFlags cullMode = VK_CULL_MODE_NONE,
	                 VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE)
	{
		rasterizationStateCreateInfo.cullMode = cullMode;
		rasterizationStateCreateInfo.frontFace = frontFace;
	}

private:
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissors;
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
	std::set<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
};

#endif // PIPELINE_H