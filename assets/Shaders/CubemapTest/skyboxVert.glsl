#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) readonly buffer VertexData
{
    vec4 vertices[];
};

layout(set=0, binding = 1) uniform Camera
{
    mat4 proj;
    mat4 view;
};

layout(location = 0) out vec3 fragUV;

void main() {
    gl_Position = proj * view * vec4(vertices[gl_VertexIndex].xyz, 1.0);
    fragUV = vertices[gl_VertexIndex].xyz;
}