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

layout(set=0, binding = 1) uniform Camera
{
    mat4 proj;
    mat4 view;
};

layout(push_constant) uniform ObjectData
{
    mat4 transform;
};

layout(location = 0) out vec3 fragUV;

void main() {
    gl_Position = proj * view * transform * vec4(vertices[gl_VertexIndex].inPosition.xyz, 1.0);
    fragUV = vertices[gl_VertexIndex].inPosition.xyz;
}