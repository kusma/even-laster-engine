#include "scenerenderer.h"

#include "vulkan.h"
#include "shader.h"

#include <utility>
#include <map>

using namespace vulkan;

using std::make_pair;
using std::map;
using std::shared_ptr;
using glm::mat4;

struct PerObjectUniforms {
	mat4 modelViewProjectionMatrix;
	mat4 modelViewProjectionInverseMatrix;
};

VkPipeline SceneRenderer::createGraphicsPipeline(const ShaderProgram *shaderProgram, VkRenderPass renderPass, const VkPipelineVertexInputStateCreateInfo &pipelineVertexInputStateCreateInfo)
{
	assert(shaderProgram != nullptr);

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
	pipelineCreateInfo.layout = shaderProgram->getPipelineLayout();
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

	auto shaderStages = shaderProgram->getPipelineShaderStageCreateInfos();
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();

	VkPipeline pipeline;
	auto err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
	assert(err == VK_SUCCESS);

	return pipeline;
}

SceneRenderer::SceneRenderer(Scene *scene, VkRenderPass renderPass, ShaderProgramFactory shaderProgramFactory) :
	scene(scene)
{
	map<const Mesh *, IndexedBatch> indexedBatches;
	map<const Material *, shared_ptr<ShaderProgram>> shaderPrograms;
	map<ShaderProgram *, VkDescriptorSet> descriptorSets;
	map<VertexFormat, VkPipeline> pipelines;

	for (auto object : scene->getObjects()) {
		// transform materials to shaderPrograms
		auto material = object.getModel().getMaterial();
		if (shaderPrograms.find(&material) == shaderPrograms.end()) {
			shared_ptr<ShaderProgram> shaderProgram = shaderProgramFactory(material);
			shaderPrograms.insert(make_pair(&material, shaderProgram));
		}

		// transform meshes to indexed batches
		auto mesh = object.getModel().getMesh();
		if (indexedBatches.find(&mesh) == indexedBatches.end())
			indexedBatches.insert(make_pair(&mesh, meshToIndexedBatch(mesh)));

		// transform vertexformats to pipelines
		auto vertexFormat = mesh.getVertexFormat();
		if (pipelines.find(vertexFormat) == pipelines.end()) {
			VkVertexInputBindingDescription vertexInputBindingDesc[1];
			vertexInputBindingDesc[0].binding = 0;
			vertexInputBindingDesc[0].stride = mesh.getVertexStride();
			vertexInputBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			auto vertexInputAttributeDescriptions = vertexFormatToInputAttributeDescriptions(vertexFormat);

			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = ARRAY_SIZE(vertexInputBindingDesc);
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDesc;
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

			auto pipeline = createGraphicsPipeline(shaderPrograms[&material].get(), renderPass, pipelineVertexInputStateCreateInfo);
			pipelines.insert(make_pair(vertexFormat, pipeline));
		}
	}

	// TODO: derive from shaderPrograms
	auto descriptorPool = createDescriptorPool({
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 },
		}, 1);

	uniformBufferSpacing = uint32_t(alignSize(sizeof(PerObjectUniforms), deviceProperties.limits.minUniformBufferOffsetAlignment));
	auto uniformBufferSize = VkDeviceSize(uniformBufferSpacing * scene->getTransforms().size());

	uniformBuffer = new Buffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VkDescriptorBufferInfo descriptorBufferInfo = uniformBuffer->getDescriptorBufferInfo();

	for (auto shaderProgram : shaderPrograms) {
		auto descriptorSet = allocateDescriptorSet(descriptorPool, shaderProgram.second->getDescriptorSetLayout());

		// TODO: this seems wrong. Should probably be done in some other way.
		VkWriteDescriptorSet writeDescriptorSets[1] = {};
		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet = descriptorSet;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfo;
		writeDescriptorSets[0].dstBinding = 0;

		vkUpdateDescriptorSets(device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
		descriptorSets[shaderProgram.second.get()] = descriptorSet;
	}

	auto offset = 0u;
	map<const Transform*, unsigned int> offsetMap;
	auto transforms = scene->getTransforms();
	for (auto transform : transforms) {
		offsetMap[transform] = offset;
		offset += uniformBufferSpacing;
	}

	for (auto object : scene->getObjects()) {
		auto material = object.getModel().getMaterial();
		auto mesh = object.getModel().getMesh();
		auto shaderProgram = shaderPrograms.find(&material)->second;
		auto indexedBatch = indexedBatches.find(&mesh)->second;
		auto pipeline = pipelines[mesh.getVertexFormat()];

		Object obj = {
			indexedBatch,
			pipeline,
			shaderProgram->getPipelineLayout(),
			descriptorSets[shaderProgram.get()],
			offsetMap[&object.getTransform()]
		};
		objects.push_back(obj);
	}
}

void SceneRenderer::draw(VkCommandBuffer commandBuffer, const mat4 &viewProjectionMatrix)
{
	auto offset = 0u;
	map<const Transform*, uint32_t> offsetMap;
	auto transforms = scene->getTransforms();
	auto ptr = uniformBuffer->map(0, uniformBufferSpacing * transforms.size());
	for (auto transform : transforms) {
		auto modelMatrix = transform->getAbsoluteMatrix();
		auto modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;
		auto modelViewProjectionInverseMatrix = glm::inverse(modelViewProjectionMatrix);

		PerObjectUniforms perObjectUniforms;
		perObjectUniforms.modelViewProjectionMatrix = modelViewProjectionMatrix;
		perObjectUniforms.modelViewProjectionInverseMatrix = modelViewProjectionInverseMatrix;

		memcpy(static_cast<uint8_t *>(ptr) + offset, &perObjectUniforms, sizeof(perObjectUniforms));

		assert(offset < UINT32_MAX);
		offsetMap[transform] = (uint32_t)offset;
		offset += uniformBufferSpacing;
	}
	uniformBuffer->unmap();

	for (auto obj : objects) {
		obj.indexedBatch.bind(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.pipeline);

		assert(obj.offset <= uniformBuffer->getSize() - sizeof(PerObjectUniforms));
		uint32_t dynamicOffsets[] = { obj.offset };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.pipelineLayout, 0, 1, &obj.descriptorSet, 1, dynamicOffsets);
		obj.indexedBatch.draw(commandBuffer);
	}
}
