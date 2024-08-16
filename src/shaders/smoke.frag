#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec4 inColor;
layout (location = 0) out vec4 outFragColor;

void main()
{
	float d = 2 * distance(inTexCoord, vec2(0.5));
	vec3 col = vec3(max(1 - d, 0) * 0.015);
	outFragColor = vec4(col, 1) * inColor.r;
}
