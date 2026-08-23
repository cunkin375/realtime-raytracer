struct Metadata
{
    float3 camera_position;
    float3 ray_direction;
    float3 background;
    float image_width;
    float frame_index;
};
[[vk::binding(0, 0)]] ConstantBuffer<Metadata> meta_buffer : register(b0, space0);

struct Sphere
{
    float3 posittion;
    float radius;
    int material_index;
};
[[vk::binding(0, 1)]] StructuredBuffer<Sphere> spheres : register(t0, space0);

struct Material
{
};
[[vk::binding(0, 2)]] StructuredBuffer<Material> materials : register(t0, space0);

struct HitRecord
{
    float4 world_position;
    float4 world_normal;
    float hit_distance;
    int object_index;
};

struct Ray
{
    float4 origin;
    float4 direction;
};


// Vulkan Bindings
// - these are likely temporary
// - image_data should ideally not be linked with CPU at this stage
[[vk::binding(0, 3)]] RWStructuredBuffer<float4> accumulation_data : register(u0, space0);
[[vk::binding(0, 4)]] RWStructuredBuffer<float4> image_data : register(u1, space0);

// Returns a random seed between 0 and 1
uint PCG_Hash(uint input)
{
    uint state = input * 747796405u + 289133643u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803747u;
    return (word >> 22u) ^ word;
}

float RandomUnitInterval(uint seed)
{
    const int IEEE_mantissa = 0x007fffff;
    const int IEEE_one = 0x3f800000;
    seed = PCG_Hash(seed);
    // mask seed into floating-point mantissa
    uint result = (seed & IEEE_mantissa) | IEEE_one;
    // treat as float and return in [0, 1) range
    // subtraction of 1.0f is needed to include 0
    return asfloat(result) - 1.0f;
}


float3 RandomUnitSphereVector(uint seed)
{
    float x = RandomUnitInterval(seed);
    float y = RandomUnitInterval(seed);
    float z = RandomUnitInterval(seed);
    return normalize(float3(x, y, z));
}

HitRecord TraceRay(const Ray ray)
{
    HitRecord record;
    return record;
}

HitRecord ClosestHit(const Ray ray, float hit_distance, int object_index)
{
    HitRecord record;
    return record;
}

HitRecord Miss(const Ray ray)
{
    HitRecord record;
    return record;
}

// Generates a random vector with sampled color information
float4 RayGen(uint x, uint y)
{
    Ray ray;
    ray.origin = meta_buffer.camera_position;
    ray.direction = meta_buffer.ray_direction;

    // a ray's light starts at 0 and accumulates light from light sources
    float3 light = float3(0.f);
    // it fully contributes collor across all channels, but diminishes as it hits materials that absorb color
    float3 color_contribution = float3(1.f);
    // together, light and color_contribution are implemented to support shadows through difussed lighting

    uint seed = x + y * meta_buffer.image_width;

    // ray bounces should be kept low
    // - the GPU does not like dynamic branching in a for loop
    uint bounces = 8;
    for (uint i = 0; i < 8; i++)
    {
        seed += i;

        HitRecord record = TraceRay(ray);
        if (record.hit_distance < 0.f)
        {
            light += meta_buffer.background;
            break;
        }

        // const Sphere closest_sphere =
    }

    return float4(0, 0, 0, 0);
}

float4 ConvertToRGBA(float4 color)
{
    return float4(0, 0, 0, 0);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint x = id.x;
    uint y = id.y;
    float4 pixel_color = RayGen(x, y);
    accumulation_data[x + y * meta_buffer.image_width] += pixel_color;

    float4 accumulated_color = accumulation_data[x + y * meta_buffer.image_width];
    accumulated_color = accumulated_color / meta_buffer.frame_index;

    accumulated_color = clamp(accumulated_color, 0.f, 1.f);
    image_data[x + y * meta_buffer.image_width] = ConvertToRGBA(accumulated_color);
}

