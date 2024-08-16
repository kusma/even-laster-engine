#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConstants {
	float time;
} pushConstants;

layout (binding = 0) uniform sampler2D samplers[2];

void main()
{
	float time = pushConstants.time * 0.1;
	vec4 bg = texture(samplers[0], inTexCoord + vec2(sin(time), cos(time * 1.15)) * 0.01);
	vec4 overlay = texture(samplers[1], inTexCoord + vec2(sin(time * 0.95), cos(time * 1.10)) * 0.0025);

	vec4 color = bg * (1 - overlay.a) + overlay;

	outFragColor = color;
}
