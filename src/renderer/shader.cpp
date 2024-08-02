#include "shader.h"
#include "core/memorymappedfile.h"

#include "vkhelpers.h"
#include "vkinstance.h"

VkShaderModule loadShaderModule(const char *path)
{
	MemoryMappedFile shaderCode(path);
	assert(shaderCode.getSize() > 0);

	VkShaderModuleCreateInfo moduleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shaderCode.getSize(),
		.pCode = static_cast<const uint32_t *>(shaderCode.getData()),
	};

	VkShaderModule shaderModule;
	vkHelpers::assumeSuccess(vkCreateShaderModule(vkInstance::device, &moduleCreateInfo, nullptr, &shaderModule));

	return shaderModule;
}
