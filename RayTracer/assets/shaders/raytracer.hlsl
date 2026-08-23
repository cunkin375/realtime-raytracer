struct Sphere
{
    float3 posittion;
    float1 radius;
    int1 material_index;
};

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

struct Metadata
{
    float3 camera_position;
    float3 ray_direction;
    float1 image_width;
    float1 frame_index;
};

// Vulkan Bindings
[[vk::binding(0)]] ConstantBuffer<Metadata> meta_buffer : register(b0);
[[vk::binding(3)]] RWStructuredBuffer<float4> accumulation_data : register(u0);
[[vk::binding(4)]] RWStructuredBuffer<float4> image_data : register(u1);

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

float4 RayGen(uint x, uint y)
{
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

    accumulated_color = clamp(accumulated_color, 0.0f, 1.0f);
    image_data[x + y * meta_buffer.image_width] = ConvertToRGBA(accumulated_color);
}

