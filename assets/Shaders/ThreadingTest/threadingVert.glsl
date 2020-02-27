#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
    vec4 inPosition;
    vec4 inNormal;
    vec4 inUv;
};

layout(set=0, binding = 0) readonly buffer VertexData
{
    Vertex vertices[];
};

layout(set=0, binding = 1) uniform Camera { mat4 vp; };

layout(push_constant) uniform TintColor
{
    mat4 mw;
    vec4 tint; // Could be in a push_constant in the fragment shader.
};

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUv;
layout(location = 2) out vec4 fragTint;

void main() {
    gl_Position = vp * mw * vec4(vertices[gl_VertexIndex].inPosition.xyz, 1.0);
    fragNormal = normalize((mw * vec4(vertices[gl_VertexIndex].inNormal.xyz, 0.0)).xyz);
    fragUv = vertices[gl_VertexIndex].inUv.xy;
    fragTint = tint;
}