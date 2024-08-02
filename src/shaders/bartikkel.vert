#version 450
#extension GL_GOOGLE_include_directive : enable

#include "bartikkel.glsl"
#include "utils.glsl"

layout (location = 0) out float outSize;
layout (binding = 1) uniform sampler3D volumeSampler;

void main()
{
	int strip = gl_InstanceIndex;
	int stripIndex = gl_VertexIndex;
	vec2 texCoord = vec2(stripIndex, strip);
	texCoord = (texCoord - 127.5) / 128;
	texCoord *= 0.1;
	vec3 pos = vec3(ubo.offset + ubo.scale * texCoord, ubo.time);

	pos = textureLod(volumeSampler, pos, 0).xyz;
	pos += textureLod(volumeSampler, pos * 3, 0).xyz * 0.5;
	pos += textureLod(volumeSampler, pos * 15, 0).xyz * 0.01;
	pos *= 5;

	gl_Position = ubo.modelViewProjectionMatrix * vec4(pos, 1.0);
	outSize = 0.05 * gl_Position.w;
}
