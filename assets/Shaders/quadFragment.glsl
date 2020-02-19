#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D tex;

float mask[4] = {0.22508352, 0.11098164, 0.01330373, 0.00038771};
float pixelSize = 0.00078125;

void main() {
	vec4 sampled = texture(tex, fragUV);
	for(int i = 0; i < 4; i++) {
		float xSample = fragUV.x + (i+1) * pixelSize * 6;
		sampled += mask[i] * texture(tex, vec2(xSample, fragUV.y));
	}
		for(int i = 0; i < 4; i++) {
		float ySample = fragUV.y + (i+1) * pixelSize * 6;
		sampled += mask[i] * texture(tex, vec2(fragUV.x, ySample));
	}

	// vec4 sampled = texture(tex, fragUV);
	outColor = sampled;
}