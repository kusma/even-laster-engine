#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "blur.glsl"

layout (location = 0) in vec2 texCoord;
layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform sampler2D textureSampler;

void main()
{
	outFragColor = fastBlur5x5(textureSampler, texCoord, 0);
}
