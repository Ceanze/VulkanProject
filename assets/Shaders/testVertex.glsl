#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 fragUV;

layout(set=0, binding = 0) readonly buffer Data
{
    vec2 uv[3];
    vec2 positions[3];
};

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragUV = uv[gl_VertexIndex];
}