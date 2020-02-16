#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) readonly buffer Positions
{
    vec3 positions[1000];
};

layout(set=0, binding = 1) uniform Mvp
{
    mat4 mvp;
};


void main() {
    gl_Position = mvp*vec4(positions[gl_VertexIndex], 1.0);
}