#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec4 fragTint;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4((fragNormal+1)/2.0, 1.0) * fragTint + vec4(fragUv*0.0001, 0.0, 0.0);
}