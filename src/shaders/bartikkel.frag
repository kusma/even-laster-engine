#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;
layout (binding = 2) uniform sampler2D samp;

void main()
{
	outFragColor = texture(samp, inTexCoord) * inColor.a;
}
