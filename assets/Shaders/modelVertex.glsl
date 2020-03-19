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

layout(set=0, binding = 1) readonly buffer TransformData
{
    mat4 modelTransform[];
};

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUv;


layout(push_constant) uniform PushConstants
{
    mat4 transform;
};

layout(set=0, binding = 2) uniform Camera
{
    mat4 vp;
};

void main() {
    gl_Position = vp * transform * modelTransform[gl_InstanceIndex] * vec4(vertices[gl_VertexIndex].inPosition.xyz, 1.0);
    fragNormal = normalize((transform * modelTransform[gl_InstanceIndex] * vec4(vertices[gl_VertexIndex].inNormal.xyz, 0.0)).xyz);
    fragUv = vertices[gl_VertexIndex].inUv.xy;
}