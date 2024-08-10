#include "scenerenderer.h"

#include "vkhelpers.h"
#include "shader.h"
#include "pipeline.h"
#include "descriptorset.h"

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

static VkPipeline createGraphicsPipeline(VkPipelineLayout layout, const RenderPass &renderPass, const VkPipelineVertexInputStateCreateInfo &pipelineVertexInputStateCreateInfo, const std::vector<VkPipelineShaderStageCreateInfo> shaderStages)
{
	GraphicsPipelineBuilder pb;

	for (const auto shaderStage: shaderStages)
		pb.addShaderStage(shaderStage);

	for (unsigned i = 0; i < pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount; ++i)
		pb.addVertexInputBinding(pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions[i]);

	for (unsigned i = 0; i < pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount; ++i)
		pb.addVertexInputAttribute(pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions[i]);

	pb.setIAState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pb.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
	pb.addColorBlendAttachment({
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	});
	pb.setRasterizationSamples(renderPass.getSampleCount());

	pb.addViewport({}); // dummy
	pb.addScissor({}); // dummy

	pb.setDepthTest(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	pb.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	pb.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);

	return pb.createPipeline(layout, renderPass.getRenderPass());
}

static IndexedBatch *meshToIndexedBatch(const Mesh &mesh)
{
	auto vertices = mesh.getVertices();
	auto indices = mesh.getIndices();

	auto vertexStagingBuffer = vkInstance::getStagingBuffer(vertices.size());
	vertexStagingBuffer->uploadMemory(vertices.data(), vertices.size());

	auto indexStagingBuffer = vkInstance::getStagingBuffer(indices.size());
	indexStagingBuffer->uploadMemory(indices.data(), indices.size());

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

	auto ret = new IndexedBatch(indexType, indexCount);

	auto vertexBuffer = ret->createVertexBuffer(vertices.size(), 0);
	auto indexBuffer = ret->createIndexBuffer(indices.size());

	vkInstance::submitSetupCommands([&](VkCommandBuffer commandBuffer) {
		vertexBuffer->uploadFromStagingBuffer(commandBuffer, vertexStagingBuffer,
		                                      0, 0, vertices.size());
		indexBuffer->uploadFromStagingBuffer(commandBuffer, indexStagingBuffer,
		                                     0, 0, indices.size());
	});

	return ret;
}

static vector<VkVertexInputAttributeDescription> vertexFormatToInputAttributeDescriptions(VertexFormat vertexFormat)
{
	vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
	unsigned offset = 0;

	if (vertexFormat & VERTEX_FORMAT_POSITION) {
		VkVertexInputAttributeDescription attr = {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offset,
		};
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	if (vertexFormat & VERTEX_FORMAT_NORMAL) {
		VkVertexInputAttributeDescription attr = {
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offset,
		};
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	if (vertexFormat & VERTEX_FORMAT_TANGENT) {
		VkVertexInputAttributeDescription attr = {
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offset,
		};
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	if (vertexFormat & VERTEX_FORMAT_BINORMAL) {
		VkVertexInputAttributeDescription attr = {
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offset,
		};
		vertexInputAttributeDescriptions.push_back(attr);
		offset += sizeof(float) * 3;
	}

	return vertexInputAttributeDescriptions;
}

VertexBuffer *IndexedBatch::createVertexBuffer(VkDeviceSize size, VkDeviceSize offset)
{
	auto vb = new VertexBuffer(size);
	vertexBuffers.push_back(vb->getBuffer());
	vertexBufferOffsets.push_back(offset);
	buffers.push_back(vb);
	return vb;
}

IndexBuffer *IndexedBatch::createIndexBuffer(VkDeviceSize size)
{
	assert(indexBuffer == VK_NULL_HANDLE);
	auto ib = new IndexBuffer(size);
	indexBuffer = ib->getBuffer();
	buffers.push_back(ib);
	return ib;
}

IndexedBatch::~IndexedBatch()
{
	for (auto buf : buffers) {
		delete buf;
	}
}

SceneRenderer::SceneRenderer(const Scene *scene, const RenderPass &renderPass) :
	scene(scene)
{
	DescriptorSetBuilder descSetBuilder;
	descSetBuilder.addUniformBufferDynamic(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	descSetBuilder.addCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
	descSetBuilder.addCombinedImageSampler(2, VK_SHADER_STAGE_FRAGMENT_BIT);
	descSetBuilder.addUniformBuffer(3, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto descriptorSetLayout = descSetBuilder.createDescriptorSetLayout();

	pipelineLayout = createPipelineLayout(vkInstance::device, { descriptorSetLayout }, {});

	for (auto object : scene->getObjects()) {
		// transform meshes to indexed batches
		auto mesh = object->getModel()->getMesh();
		if (indexedBatches.find(mesh) == indexedBatches.end()) {
			// FIXME: currently leaks all IndexedBatch objects
			indexedBatches.insert(std::make_pair(mesh, meshToIndexedBatch(*mesh)));
		}

		// transform vertexformats to pipelines
		auto vertexFormat = mesh->getVertexFormat();
		if (pipelines.find(vertexFormat) == pipelines.end()) {
			auto stride = Mesh::calculateVertexStride(vertexFormat);
			assert(stride < UINT32_MAX);
			VkVertexInputBindingDescription vertexInputBindingDesc[1] = {{
				.binding = 0,
				.stride = uint32_t(stride),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			}};

			auto vertexInputAttributeDescriptions = vertexFormatToInputAttributeDescriptions(vertexFormat);
			auto vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
			assert(vertexAttributeDescriptionCount < UINT32_MAX);

			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = ARRAY_SIZE(vertexInputBindingDesc),
				.pVertexBindingDescriptions = vertexInputBindingDesc,
				.vertexAttributeDescriptionCount = uint32_t(vertexAttributeDescriptionCount),
				.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data(),
			};

			auto shaderStages = createStageVector({
				.vertexShader = loadShaderModule("data/shaders/refraction.vert.spv"),
				.fragmentShader = loadShaderModule("data/shaders/refraction.frag.spv"),
			});
			auto pipeline = createGraphicsPipeline(pipelineLayout,
			                                       renderPass,
			                                       pipelineVertexInputStateCreateInfo,
			                                       shaderStages);
			pipelines.insert(std::make_pair(vertexFormat, pipeline));
		}
	}

	auto descriptorPool = descSetBuilder.createDescriptorPool(1);

	uniformBufferSpacing = uint32_t(alignSize(sizeof(PerObjectUniforms), vkInstance::deviceProperties.limits.minUniformBufferOffsetAlignment));
	auto uniformBufferSize = VkDeviceSize(uniformBufferSpacing * scene->getTransforms().size());

	uniformBuffer = new UniformBuffer(uniformBufferSize);

	descriptorSet = allocateDescriptorSet(vkInstance::device, descriptorPool, descriptorSetLayout);

	VkDescriptorBufferInfo descriptorBufferInfo = uniformBuffer->getDescriptorBufferInfo(0, uniformBufferSpacing);

	VkWriteDescriptorSet writeDescriptorSets[1] = { {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.pBufferInfo = &descriptorBufferInfo,
	} };

	vkUpdateDescriptorSets(vkInstance::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
}

SceneRenderer::~SceneRenderer()
{
	for (const auto& v : indexedBatches) {
		delete v.second;
	}
	indexedBatches.clear();

	for (const auto& v : pipelines) {
		vkDestroyPipeline(vkInstance::device, v.second, nullptr);
	}
	pipelines.clear();
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

		PerObjectUniforms perObjectUniforms = {
			.modelViewMatrix = modelViewMatrix,
			.modelViewInverseMatrix = glm::inverse(modelViewMatrix),
			.modelViewProjectionMatrix = modelViewProjectionMatrix,
		};

		memcpy(static_cast<uint8_t *>(ptr) + offset, &perObjectUniforms, sizeof(perObjectUniforms));
		offsetMap[transform] = offset;
		offset += uniformBufferSpacing;
	}
	uniformBuffer->unmap();

	for (auto object : scene->getObjects()) {
		auto mesh = object->getModel()->getMesh();
		auto indexedBatch = indexedBatches.find(mesh)->second;
		auto pipeline = pipelines[mesh->getVertexFormat()];

		indexedBatch->bind(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		assert(offsetMap.count(object->getTransform()) > 0);

		auto offset = offsetMap[object->getTransform()];
		assert(offset <= uniformBuffer->getSize() - sizeof(PerObjectUniforms));
		uint32_t dynamicOffsets[] = { (uint32_t)offset };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, dynamicOffsets);
		indexedBatch->draw(commandBuffer);
	}
}
