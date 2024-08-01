#include "scenerenderer.h"

#include "vkhelpers.h"
#include "shader.h"

#include "buffer.h"

#include <utility>
#include <map>
#include <vector>

using namespace vkHelpers;

using std::map, std::vector;

struct PerObjectUniforms {
	glm::mat4 modelViewMatrix;
	glm::mat4 modelViewInverseMatrix;
	glm::mat4 modelViewProjectionMatrix;
};

static VkPipeline createGraphicsPipeline(VkPipelineLayout layout, VkRenderPass renderPass, const VkPipelineVertexInputStateCreateInfo &pipelineVertexInputStateCreateInfo, const std::vector<VkPipelineShaderStageCreateInfo> shaderStages)
{
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState[1] = { { 0 } };
	pipelineColorBlendAttachmentState[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState[0].blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.attachmentCount = ARRAY_SIZE(pipelineColorBlendAttachmentState);
	pipelineColorBlendStateCreateInfo.pAttachments = pipelineColorBlendAttachmentState;

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports = nullptr;
	pipelineViewportStateCreateInfo.scissorCount = 1;
	pipelineViewportStateCreateInfo.pScissors = nullptr;

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkDynamicState dynamicStateEnables[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables;
	pipelineDynamicStateCreateInfo.dynamicStateCount = ARRAY_SIZE(dynamicStateEnables);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();

	VkPipeline pipeline;
	assumeSuccess(vkCreateGraphicsPipelines(vkInstance::device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

	return pipeline;
}

static IndexedBatch meshToIndexedBatch(const Mesh &mesh)
{
	auto vertices = mesh.getVertices();
	auto indices = mesh.getIndices();

	auto vertexStagingBuffer = new StagingBuffer(vertices.size());
	vertexStagingBuffer->uploadMemory(vertices.data(), vertices.size());

	auto vertexBuffer = new Buffer(vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, 0);
	vertexBuffer->uploadFromStagingBuffer(vertexStagingBuffer, 0, 0, vertices.size());

	auto indexStagingBuffer = new StagingBuffer(indices.size());
	indexStagingBuffer->uploadMemory(indices.data(), indices.size());

	auto indexBuffer = new Buffer(indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, 0);
	indexBuffer->uploadFromStagingBuffer(indexStagingBuffer, 0, 0, indices.size());

	VkIndexType indexType = VK_INDEX_TYPE_UINT16; // dummy
	uint32_t indexCount = 0;
	switch (mesh.getIndexType()) {
	case INDEX_TYPE_UINT16:
		indexType = VK_INDEX_TYPE_UINT16;
		indexCount = indices.size() / sizeof(uint16_t);
		break;

	case INDEX_TYPE_UINT32:
		indexType = VK_INDEX_TYPE_UINT32;
		indexCount = indices.size() / sizeof(uint32_t);
		break;

	default:
		unreachable("invalid index-type!");
	}

	// FIXME: leaks both vertexBuffer and indexBuffer!

	return IndexedBatch(
		std::vector<VkBuffer> { vertexBuffer->getBuffer() },
		std::vector<VkDeviceSize> { 0 },
		indexBuffer->getBuffer(),
		indexType,
		indexCount);
}

static vector<VkVertexInputAttributeDescription> vertexFormatToInputAttributeDescriptions(VertexFormat vertexFormat)
{
	vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
	int offset = 0;

	if (vertexFormat & VERTEX_FORMAT_POSITION) {
		VkVertexInputAttributeDescription attr;
		attr.binding = 0;
		attr.location = 0;
		attr.format = VK_FORMAT_R32G32B32_SFLOAT;
		attr.offset = offset;
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	if (vertexFormat & VERTEX_FORMAT_NORMAL) {
		VkVertexInputAttributeDescription attr;
		attr.binding = 0;
		attr.location = 1;
		attr.format = VK_FORMAT_R32G32B32_SFLOAT;
		attr.offset = offset;
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	if (vertexFormat & VERTEX_FORMAT_TANGENT) {
		VkVertexInputAttributeDescription attr;
		attr.binding = 0;
		attr.location = 2;
		attr.format = VK_FORMAT_R32G32B32_SFLOAT;
		attr.offset = offset;
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	if (vertexFormat & VERTEX_FORMAT_BINORMAL) {
		VkVertexInputAttributeDescription attr;
		attr.binding = 0;
		attr.location = 3;
		attr.format = VK_FORMAT_R32G32B32_SFLOAT;
		attr.offset = offset;
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	return vertexInputAttributeDescriptions;
}

SceneRenderer::SceneRenderer(Scene *scene, VkRenderPass renderPass) :
	scene(scene)
{
	auto descriptorSetLayout = createDescriptorSetLayout(vkInstance::device, {
		{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		});
	pipelineLayout = createPipelineLayout(vkInstance::device, { descriptorSetLayout }, {});

	for (auto object : scene->getObjects()) {
		// transform meshes to indexed batches
		auto mesh = object->getModel()->getMesh();
		if (indexedBatches.find(mesh) == indexedBatches.end())
			indexedBatches.insert(std::make_pair(mesh, meshToIndexedBatch(*mesh)));

		// transform vertexformats to pipelines
		auto vertexFormat = mesh->getVertexFormat();
		if (pipelines.find(vertexFormat) == pipelines.end()) {
			VkVertexInputBindingDescription vertexInputBindingDesc[1];
			vertexInputBindingDesc[0].binding = 0;
			vertexInputBindingDesc[0].stride = mesh->getVertexStride();
			vertexInputBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			auto vertexInputAttributeDescriptions = vertexFormatToInputAttributeDescriptions(vertexFormat);

			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = ARRAY_SIZE(vertexInputBindingDesc);
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDesc;
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

			auto pipeline = createGraphicsPipeline(pipelineLayout, renderPass, pipelineVertexInputStateCreateInfo, {
				{
					VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					nullptr,
					0,
					VK_SHADER_STAGE_VERTEX_BIT,
					loadShaderModule("data/shaders/refraction.vert.spv"),
					"main",
					NULL
				},{
					VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					nullptr,
					0,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					loadShaderModule("data/shaders/refraction.frag.spv"),
					"main",
					NULL
				}});
			pipelines.insert(std::make_pair(vertexFormat, pipeline));
		}
	}

	auto descriptorPool = createDescriptorPool(vkInstance::device, {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
		}, 1);

	uniformBufferSpacing = uint32_t(alignSize(sizeof(PerObjectUniforms), vkInstance::deviceProperties.limits.minUniformBufferOffsetAlignment));
	auto uniformBufferSize = VkDeviceSize(uniformBufferSpacing * scene->getTransforms().size());

	uniformBuffer = new UniformBuffer(uniformBufferSize);

	descriptorSet = allocateDescriptorSet(vkInstance::device, descriptorPool, descriptorSetLayout);

	VkDescriptorBufferInfo descriptorBufferInfo = uniformBuffer->getDescriptorBufferInfo(0, uniformBufferSpacing);

	VkWriteDescriptorSet writeDescriptorSets[1] = {};
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].dstSet = descriptorSet;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfo;
	writeDescriptorSets[0].dstBinding = 0;

	vkUpdateDescriptorSets(vkInstance::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
}

void SceneRenderer::draw(VkCommandBuffer commandBuffer, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix)
{
	auto offset = 0u;
	map<const Transform*, unsigned int> offsetMap;
	auto transforms = scene->getTransforms();
	auto ptr = uniformBuffer->map();

	for (auto transform : transforms) {
		auto modelMatrix = glm::mat4(1); //  transform->getAbsoluteMatrix();
		auto modelViewMatrix = viewMatrix * modelMatrix;
		auto modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;

		PerObjectUniforms perObjectUniforms;
		perObjectUniforms.modelViewMatrix = modelViewMatrix;
		perObjectUniforms.modelViewInverseMatrix = glm::inverse(modelViewMatrix);
		perObjectUniforms.modelViewProjectionMatrix = modelViewProjectionMatrix;

		memcpy(static_cast<uint8_t *>(ptr) + offset, &perObjectUniforms, sizeof(perObjectUniforms));
		offsetMap[transform] = offset;
		offset += uniformBufferSpacing;
	}
	uniformBuffer->unmap();

	for (auto object : scene->getObjects()) {
		auto mesh = object->getModel()->getMesh();
		auto indexedBatch = indexedBatches.find(mesh)->second;
		auto pipeline = pipelines[mesh->getVertexFormat()];

		indexedBatch.bind(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		assert(offsetMap.count(object->getTransform()) > 0);

		auto offset = offsetMap[object->getTransform()];
		assert(offset <= uniformBuffer->getSize() - sizeof(PerObjectUniforms));
		uint32_t dynamicOffsets[] = { (uint32_t)offset };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, dynamicOffsets);
		indexedBatch.draw(commandBuffer);
	}
}
