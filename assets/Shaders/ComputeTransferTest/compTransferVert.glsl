#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragPos;

layout(set = 0, binding = 0, std430) readonly buffer Positions
{
    vec4 inPosition[];
};

layout(set=0, binding = 1) uniform Camera
{
    mat4 vp;
};

void main() {
    fragPos = vec4(inPosition[gl_VertexIndex].xyz, 1.0);;
    gl_Position = vp * fragPos;
} 