#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "utils.glsl"
#include "perobject.glsl"

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec3 modelPos;
layout (location = 2) in vec3 inNormal;


layout (location = 0) out vec4 outFragColor;

layout (binding = 1) uniform samplerCube samplerEnv;
layout (binding = 2) uniform UBO
{
	float planeIndex;
	float fade;
	float refractiveIndex;
} ubo;

void main()
{
	vec3 modelNormal = normalize(inNormal); // normalize(cross(dFdx(modelPos), dFdy(modelPos)));
	vec3 pos = modelPos;
	vec3 viewPos = perObjectUBO.modelViewInverseMatrix[3].xyz;
	vec3 view = normalize(modelPos - viewPos);

	vec3 color = vec3(0);

	float fres = pow(1 - dot(modelNormal, -view), 3);
	vec3 envCoord = reflect(normalize(view), modelNormal);
	color += (0.1 + 0.5 * texture(samplerEnv, envCoord).rgb * pow(1 - dot(modelNormal, -view), 3)) * fres;

	outFragColor = vec4(color, 1);
}
