#version 450
#extension GL_GOOGLE_include_directive : enable

#include "bartikkel.glsl"

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (location = 0) in float inSize[];

layout (location = 0) out vec2 outTexCoord;

void main() {
    vec4 offs = vec4(ubo.offsets.xy, -ubo.offsets.xy) * inSize[0];

    gl_Position = gl_in[0].gl_Position;
    gl_Position.xy += offs.xy;
    outTexCoord = vec2(0, 0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position;
    gl_Position.xy += offs.xw;
    outTexCoord = vec2(0, 1);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position;
    gl_Position.xy += offs.zy;
    outTexCoord = vec2(1, 0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position;
    gl_Position.xy += offs.zw;
    outTexCoord = vec2(1, 1);
    EmitVertex();

    EndPrimitive();
}
