#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUv;
layout(location = 3) in vec3 fragCamPos;

layout(set=1, binding=0) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 eye = normalize(fragPos - fragCamPos);
    vec3 r = reflect(eye, normalize(fragNormal));
    outColor = vec4(texture(skybox, r).rgb, 1.0) + vec4(fragUv*0.0001, 0.0, 0.0);
}