layout (binding = 0) uniform UBO {
	mat4 modelViewProjectionMatrix;
	vec2 offsets;

	vec2 offset;
	vec2 scale;
	float time;
} ubo;
