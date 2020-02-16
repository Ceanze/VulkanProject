#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 fragUv;

layout(set=0, binding = 0) uniform Mvp
{
    mat4 mvp;
};


void main() {
    gl_Position = mvp*vec4(inPosition, 1.0);
    fragUv = inUv;
}