#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 fragUv;

layout(set=0, binding = 0) uniform UboData
{
    mat4 world;
    mat4 view;
    mat4 proj;
};


void main() {
    gl_Position = proj * view * world * vec4(inPosition, 1.0);
    fragUv = inUv;
}