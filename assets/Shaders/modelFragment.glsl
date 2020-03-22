#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

layout(set=1, binding=0) uniform sampler2D baseColorTexture;
layout(set=1, binding=1) uniform sampler2D metallicRoughnessTexture;
layout(set=1, binding=2) uniform sampler2D normalTexture;
layout(set=1, binding=3) uniform sampler2D occlusionTexture;
layout(set=1, binding=4) uniform sampler2D emissiveTexture;

layout(push_constant) uniform PushConstantsFrag
{
    layout(offset = 64)  vec4 baseColorFactor;
	layout(offset = 80)  vec4 emissiveFactor;
	layout(offset = 96)  float metallicFactor;
	layout(offset = 100) float roughnessFactor;
	layout(offset = 104) int baseColorTextureCoord;
	layout(offset = 108) int metallicRoughnessTextureCoord;
	layout(offset = 112) int normalTextureCoord;
	layout(offset = 116) int occlusionTextureCoord;
	layout(offset = 120) int emissiveTextureCoord;
    layout(offset = 124) int padding;
};

void main() {
    vec3 baseColor = texture(baseColorTexture, fragUv).rgb;

    vec3 lightDir = normalize(vec3(0.5, -2.0, 0.5));
    float diffuse = max(dot(normalize(fragNormal), -lightDir), 0.0);
    
    outColor = vec4(baseColor*diffuse, 1.0);
}