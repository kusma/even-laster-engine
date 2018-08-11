#include "shader.h"
#include "core/memorymappedfile.h"

using namespace vulkan;

VkShaderModule loadShaderModule(const char *path)
{
	MemoryMappedFile shaderCode(path);
	assert(shaderCode.getSize() > 0);

	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = shaderCode.getSize();
	moduleCreateInfo.pCode = static_cast<const uint32_t *>(shaderCode.getData());

	VkShaderModule shaderModule;
	assumeSuccess(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));

	return shaderModule;
}
