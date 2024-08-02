#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

void main()
{
	vec3 col = vec3(1);
	outFragColor = vec4(col, 1);
}
