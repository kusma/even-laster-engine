#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include <volk.h>
#include "scene/scene.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <map>

class Scene;
class Mesh;
class Buffer;
class VertexBuffer;
class IndexBuffer;

struct IndexedBatch {
public:
	IndexedBatch(VkIndexType indexType, uint32_t indexCount) :
		indexBuffer(VK_NULL_HANDLE),
		indexType(indexType),
		indexCount(indexCount)
	{
		assert(vertexBuffers.size() == vertexBufferOffsets.size());
	}

	~IndexedBatch();

	VertexBuffer *createVertexBuffer(VkDeviceSize size, VkDeviceSize offset);
	IndexBuffer *createIndexBuffer(VkDeviceSize size);

	void bind(VkCommandBuffer commandBuffer)
	{
		assert(indexBuffer != VK_NULL_HANDLE);
		vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), vertexBufferOffsets.data());
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, indexType);
	}

	void draw(VkCommandBuffer commandBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}

private:
	std::vector<Buffer*> buffers;

	std::vector<VkBuffer> vertexBuffers;
	std::vector<VkDeviceSize> vertexBufferOffsets;
	VkBuffer indexBuffer;
	VkIndexType indexType;
	uint32_t indexCount;
};

class SceneRenderer {
public:
	SceneRenderer(const Scene *scene, VkRenderPass renderPass, VkSampleCountFlagBits sampleCount);
	~SceneRenderer();

	void draw(VkCommandBuffer commandBuffer, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix);

	VkDescriptorSet getDescriptorSet() { return descriptorSet; } // HACK! Yuck yuck yuck!

private:
	const Scene *scene;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	std::map<const Mesh *, IndexedBatch *> indexedBatches;
	std::map<VertexFormat, VkPipeline> pipelines;
	VkSampler textureSampler;

	Buffer *uniformBuffer;
	uint32_t uniformBufferSpacing;
};

#endif
