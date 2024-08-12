#version 450
#extension GL_GOOGLE_include_directive : enable

#include "particle.glsl"

layout (binding = 1) uniform bartikkelUBO {
	ivec4 xaxis;
	ivec4 yaxis;
	ivec4 zaxis;
	ivec4 zpos;
	mat4 modelViewMatrix;
} bartikkel_ubo;

#include "utils.glsl"

layout (location = 0) out float outSize;
layout (location = 1) out vec4 outColor;

#define AXIS_BITS 6
#define AXIS_MASK ((1 << AXIS_BITS) - 1)

void main()
{
	int x = gl_VertexIndex & AXIS_MASK;
	int y = (gl_VertexIndex >> AXIS_BITS) & AXIS_MASK;
	int z = (gl_VertexIndex >> (2 * AXIS_BITS)) & AXIS_MASK;

	ivec3 ipos = bartikkel_ubo.zpos.xyz;
	ipos += bartikkel_ubo.xaxis.xyz * x;
	ipos += bartikkel_ubo.yaxis.xyz * y;
	ipos += bartikkel_ubo.zaxis.xyz * z;

	vec3 pos = ipos;

	vec3 offs = vec3(randf(ipos.y + ipos.z), randf(ipos.x + ipos.z), randf(ipos.x + ipos.y));
	pos += offs - 0.5;

	float depth = (bartikkel_ubo.modelViewMatrix * vec4(pos, 1.0)).z;
	gl_Position = ubo.modelViewProjectionMatrix * vec4(pos, 1.0);

	outSize = 0.25;

	const float fogDensity = 0.1;
	outColor = vec4(vec3(1), exp(depth * fogDensity));
}
