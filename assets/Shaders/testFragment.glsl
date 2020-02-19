#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D tex;

void main() {
	vec4 sampled = texture(tex, fragUV);
	outColor = sampled;
}