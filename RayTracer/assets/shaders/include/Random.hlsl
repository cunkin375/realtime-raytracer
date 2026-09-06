// Returns a random seed between 0 and 1
// source: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint PCG_Hash(uint input)
{
    uint state = input * 747796405u + 289133643u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803747u;
    return (word >> 22u) ^ word;
}

float RandomUnitInterval(inout uint seed)
{
    seed = PCG_Hash(seed);
    return (float)seed / (float)0xFFFFFFFF;
}

float3 RandomUnitSphereVector(inout uint seed)
{
    float x = RandomUnitInterval(seed) * 2.f - 1.f;
    float y = RandomUnitInterval(seed) * 2.f - 1.f;
    float z = RandomUnitInterval(seed) * 2.f - 1.f;
    return normalize(float3(x, y, z));
}
