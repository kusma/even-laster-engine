#include "pipeline.h"

using vkHelpers::assumeSuccess;

GraphicsPipelineBuilder::GraphicsPipelineBuilder() :
	vertexInputStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	}),
	inputAssemblyStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	}),
	rasterizationStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f,
	}),
	colorBlendStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	}),
	multisampleStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	}),
	viewportStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	}),
	depthStencilStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
	}),
	dynamicStateCreateInfo({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	}),
	pipelineCreateInfo({
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pVertexInputState = &vertexInputStateCreateInfo,
		.pInputAssemblyState = &inputAssemblyStateCreateInfo,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizationStateCreateInfo,
		.pMultisampleState = &multisampleStateCreateInfo,
		.pDepthStencilState = &depthStencilStateCreateInfo,
		.pColorBlendState = &colorBlendStateCreateInfo,
		.pDynamicState = &dynamicStateCreateInfo,
	})
{
}

VkPipeline GraphicsPipelineBuilder::createPipeline(VkPipelineLayout layout, VkRenderPass renderPass)
{
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputBindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

	colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
	colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

	viewportStateCreateInfo.viewportCount = viewports.size();
	viewportStateCreateInfo.pViewports = dynamicStateEnables.contains(VK_DYNAMIC_STATE_VIEWPORT) ?
	                                     nullptr : viewports.data();
	viewportStateCreateInfo.scissorCount = scissors.size();
	viewportStateCreateInfo.pScissors = dynamicStateEnables.contains(VK_DYNAMIC_STATE_SCISSOR) ?
	                                    nullptr : scissors.data();

	std::vector<VkDynamicState> dynamicStateVector;
	for (auto const &dynamicState: dynamicStateEnables)
		dynamicStateVector.push_back(dynamicState);

	dynamicStateCreateInfo.dynamicStateCount = dynamicStateVector.size();
	dynamicStateCreateInfo.pDynamicStates = dynamicStateVector.data();

	pipelineCreateInfo.stageCount = stages.size();
	pipelineCreateInfo.pStages = stages.data();
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;

	VkPipeline pipeline;
	assumeSuccess(vkCreateGraphicsPipelines(vkInstance::device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
	return pipeline;
}
