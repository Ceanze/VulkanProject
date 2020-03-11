#version 450

layout (local_size_x = 16, local_size_y = 1) in;

struct Config {
    uint regionCount;
    uint regionSize;
    uint proximitySize;
    uint pad2;
};

layout(set = 0, binding = 0, std430) buffer Verticies
{
    uint indicies[];
};

layout(set = 0, binding = 1) uniform ConfigData
{
    Config cfg;
};

void main()
{
    uint id = gl_GlobalInvocationID.x; // + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;
    if (id < cfg.regionCount * cfg.regionCount) {
        uint numQuads = cfg.regionSize - 1;
        uint index = id * numQuads * numQuads * 6;
        
        uint vertOffsetX = (id % cfg.regionCount) * numQuads;
        uint vertOffsetZ = (id / cfg.regionCount) * numQuads;

        for (uint z = vertOffsetZ; z < vertOffsetZ + numQuads; z++)
        {
            for (uint x = vertOffsetX; x < vertOffsetX + numQuads; x++)
            {
                // First triangle
                indicies[index++] = (z * cfg.proximitySize + x);
                indicies[index++] = ((z + 1) * cfg.proximitySize + x);
                indicies[index++] = ((z + 1) * cfg.proximitySize + x + 1);
                // Second triangle
                indicies[index++] = (z * cfg.proximitySize + x);
                indicies[index++] = ((z + 1) * cfg.proximitySize + x + 1);
                indicies[index++] = (z * cfg.proximitySize + x + 1);
            }
        }
    }

    // vertOffsetX = (vertOffsetX + numQuads) % (proximitySize - 1);
    // vertOffsetZ += numQuads;
}