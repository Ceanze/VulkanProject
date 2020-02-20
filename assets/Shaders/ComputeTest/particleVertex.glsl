#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Particle {
    vec4 position;
    vec4 velocity;
};

layout(set = 0, binding = 0, std430) readonly buffer Positions
{
    Particle particles[];
};

layout(set=0, binding = 1) uniform Camera
{
    mat4 vp;
};

void main() {
    gl_PointSize = 15.0;
    gl_Position = vp * vec4(particles[gl_VertexIndex].position.xyz, 1.0);
}