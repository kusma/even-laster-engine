#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include "vkhelpers.h"
#include "scene/scene.h"
#include "renderpass.h"

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
	IndexedBatch(VkIndexType indexType, uint32_t indexCount, uint32_t instanceCount = 1) :
		indexBuffer(VK_NULL_HANDLE),
		indexType(indexType),
		indexCount(indexCount),
		instanceCount(instanceCount)
	{
		assert(vertexBuffers.size() == vertexBufferOffsets.size());
	}

	~IndexedBatch();

	VertexBuffer *createVertexBuffer(VkDeviceSize size, VkDeviceSize offset);
	IndexBuffer *createIndexBuffer(VkDeviceSize size);

	void bind(VkCommandBuffer commandBuffer) const
	{
		assert(indexBuffer != VK_NULL_HANDLE);
		vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), vertexBufferOffsets.data());
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, indexType);
	}

	void draw(VkCommandBuffer commandBuffer) const
	{
		vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
	}

private:
	std::vector<Buffer*> buffers;

	std::vector<VkBuffer> vertexBuffers;
	std::vector<VkDeviceSize> vertexBufferOffsets;
	VkBuffer indexBuffer;
	VkIndexType indexType;
	uint32_t indexCount;
	uint32_t instanceCount;
};

class SceneRenderer {
public:
	SceneRenderer(const Scene *scene, const RenderPass &renderPass);
	~SceneRenderer();

	void draw(VkCommandBuffer commandBuffer, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix);

	std::map<const Material *, VkDescriptorSet> getDescriptorSets() { return descriptorSets; } // HACK! Yuck yuck yuck!

private:
	const Scene *scene;

	VkPipelineLayout pipelineLayout;
	std::map<const Material *, VkDescriptorSet> descriptorSets;
	std::map<const Mesh *, IndexedBatch *> indexedBatches;
	std::map<VertexFormat, VkPipeline> pipelines;
	VkSampler textureSampler;

	Buffer *uniformBuffer;
	uint32_t uniformBufferSpacing;
};

#endif
