#version 450

layout (local_size_x = 16, local_size_y = 16) in;

struct Particle {
    vec4 position;
    vec4 velocity;
};

layout(set = 0, binding = 0, std430) buffer Positions
{
    Particle particles[];
};

void main()
{
    vec4 current_pos = particles[gl_GlobalInvocationID.x].position;
    vec4 velocity = particles[gl_GlobalInvocationID.x].velocity;
    current_pos += velocity;
    if (current_pos.x > 0.95  ||
        current_pos.x < -0.95 ||
        current_pos.y > 0.95  ||
        current_pos.y < -0.95 ||
        current_pos.z > 0.95  ||
        current_pos.z < -0.95)
    {
        current_pos = -2.0 * velocity + current_pos * 0.05;
    }
    particles[gl_GlobalInvocationID.x].position = current_pos;
}