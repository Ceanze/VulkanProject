#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform TintColor
{
    vec4 tint;
};

void main() {
    outColor = tint;
}