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

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUv;
layout(location = 3) out vec3 fragCamPos;

layout(set=0, binding = 1) uniform UboData
{
    mat4 world;
    mat4 vp;
    vec4 camPos;
};

layout(push_constant) uniform ObjectData
{
    mat4 transform;
};

void main() {
    fragCamPos = camPos.xyz;
    vec4 pos = world * transform * vec4(vertices[gl_VertexIndex].inPosition.xyz, 1.0);
    fragPos = pos.xyz;
    gl_Position = vp * pos;
    fragNormal = mat3(transpose(inverse(world * transform))) * vertices[gl_VertexIndex].inNormal.xyz;
    fragUv = vertices[gl_VertexIndex].inUv.xy;
}