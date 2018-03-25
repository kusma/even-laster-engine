#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include "vulkan.h"
#include "scene/scene.h"
#include "shader.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <functional>

class Scene;
class Material;
class Mesh;
class Buffer;

class SceneRenderer {
public:
	typedef std::function<std::unique_ptr<ShaderProgram>(const Material &)> ShaderProgramFactory;

	SceneRenderer(Scene *scene, VkRenderPass renderPass, ShaderProgramFactory shaderProgramFactory);
	void draw(VkCommandBuffer commandBuffer, const glm::mat4 &viewProjectionMatrix);

	static VkPipeline createGraphicsPipeline(const ShaderProgram *shaderProgram, VkRenderPass renderPass, const VkPipelineVertexInputStateCreateInfo &pipelineVertexInputStateCreateInfo);

private:
	Scene *scene;

	struct Object {
		IndexedBatch indexedBatch;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
		uint32_t offset;
	};
	std::vector<Object> objects;

	Buffer *uniformBuffer;
	uint32_t uniformBufferSpacing;
};

#endif