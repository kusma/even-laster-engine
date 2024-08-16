#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include "vkinstance.h"
#include "vkhelpers.h"

class DescriptorSetBuilder {
public:
	DescriptorSetBuilder()
	{
	}

	void addLayoutBinding(const VkDescriptorSetLayoutBinding &binding)
	{
		layoutBindings.push_back(binding);

		// count needed descriptors per type
		auto n = descriptorTypeCounts.find(binding.descriptorType);
		if (n != descriptorTypeCounts.end())
			n->second += binding.descriptorCount;
		else
			descriptorTypeCounts[binding.descriptorType] = binding.descriptorCount;
	}

	void addCombinedImageSampler(unsigned binding,
	                             VkShaderStageFlags stageFlags,
	                             unsigned descriptorCount = 1,
	                             const VkSampler *pImmutableSamplers = nullptr) {
		addLayoutBinding({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = descriptorCount,
			.stageFlags = stageFlags,
			.pImmutableSamplers = pImmutableSamplers
		});
	}

	void addStorageImage(unsigned binding,
	                     VkShaderStageFlags stageFlags,
	                     unsigned descriptorCount = 1,
	                     const VkSampler *pImmutableSamplers = nullptr) {
		addLayoutBinding({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = descriptorCount,
			.stageFlags = stageFlags,
			.pImmutableSamplers = pImmutableSamplers
		});
	}

	void addUniformBuffer(unsigned binding,
	                      VkShaderStageFlags stageFlags,
	                      unsigned descriptorCount = 1,
	                      const VkSampler *pImmutableSamplers = nullptr) {
		addLayoutBinding({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = descriptorCount,
			.stageFlags = stageFlags,
			.pImmutableSamplers = pImmutableSamplers
		});
	}

	void addUniformBufferDynamic(unsigned binding,
	                             VkShaderStageFlags stageFlags,
	                             unsigned descriptorCount = 1,
	                             const VkSampler *pImmutableSamplers = nullptr) {
		addLayoutBinding({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = descriptorCount,
			.stageFlags = stageFlags,
			.pImmutableSamplers = pImmutableSamplers
		});
	}

	VkDescriptorSetLayout createDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags flags = 0)
	{
		VkDescriptorSetLayoutCreateInfo desciptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.flags = flags,
			.bindingCount = uint32_t(layoutBindings.size()),
			.pBindings = layoutBindings.data(),
		};

		VkDescriptorSetLayout descriptorSetLayout;
		vkHelpers::assumeSuccess(vkCreateDescriptorSetLayout(vkInstance::device, &desciptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
		return descriptorSetLayout;
	}

	VkDescriptorPool createDescriptorPool(unsigned maxSets) {
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (const auto &[key, value] : descriptorTypeCounts)
			poolSizes.push_back({ key, value * maxSets });
		return vkHelpers::createDescriptorPool(vkInstance::device, poolSizes, maxSets);
	}

private:
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	std::map<VkDescriptorType, unsigned> descriptorTypeCounts;
};

#endif // DESCRIPTORSET_H
