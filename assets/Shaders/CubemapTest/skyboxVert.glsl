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

layout(set=0, binding = 1) uniform UboData
{
    mat4 world;
    mat4 vp;
};

void main() {
    gl_Position = vp * world * vec4(vertices[gl_VertexIndex].inPosition.xyz, 1.0);
    fragNormal = normalize((world * vec4(vertices[gl_VertexIndex].inNormal.xyz, 0.0)).xyz);
    fragUv = vertices[gl_VertexIndex].inUv.xy;
}