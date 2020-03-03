#version 450

layout (local_size_x = 16, local_size_y = 1) in;

struct IndexedIndirectCommand 
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct Plane
{
    vec4 normal;
    vec4 point;
};

layout(set = 0, binding = 0, std430) writeonly buffer IndirectDraws
{
    IndexedIndirectCommand indirectDraws[];
};

layout(set = 0, binding = 1, std430) readonly buffer Vertices
{
    vec4 vertices[];
};

layout(set = 0, binding = 2) uniform WorldData
{
    uint regWidth;           // Region width in number of vertices.
    uint numIndicesPerReg;   // Number of indices per region.
    uint loadedWidth;        // Loaded world width in verticies
    float _pad;
};

layout(set = 0, binding = 3) uniform Planes
{
    Plane planes[6];  // Combination of normal (Pointing inwards) and position.
};


/*
    [   ][   ][   ]
    [013014230133 0133423523432             ]

    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #

    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
    # # # # #  # # # # #  # # # # #
*/

bool frustum(vec4 pos)
{
    for(uint i = 0; i < 6; i++)
    {
        if(dot(pos.xyz - planes[i].point.xyz, planes[i].normal.xyz) < 0.0)
            return false;
    }
    return true;
}

void main()
{
    uint id = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

    uint x = (id * (regWidth - 1)) % (loadedWidth - 1);
    uint y = ((id * (regWidth - 1)) / (loadedWidth - 1)) * (regWidth - 1);

    // Corner positions (Approximation)
    uint vtl = x + y * loadedWidth;
    uint vbr = vtl + loadedWidth * (regWidth - 1) + regWidth - 1;
    vec4 tl = vertices[vtl];
    vec4 br = vertices[vbr];
    vec4 tr = tl + vec4(float(regWidth) - 1.0, 0.0, 0.0, 0.0);
    vec4 bl = br - vec4(float(regWidth) - 1.0, 0.0, 0.0, 0.0);

    bool shouldDraw = frustum(tl) || frustum(br) || frustum(tr) || frustum(bl);
    if(shouldDraw)
    {
        indirectDraws[id].instanceCount = 1;
        indirectDraws[id].firstIndex = id * numIndicesPerReg;
        indirectDraws[id].indexCount = numIndicesPerReg;
    }
    else
    {
        indirectDraws[id].instanceCount = 0;
    }
}