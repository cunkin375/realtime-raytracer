struct Metadata
{
    float3 camera_position;
    float3 ray_direction;
    float2 viewport;
    float1 frame_index;
    float1 seed;
};
[[vk::binding(0, 0)]] ConstantBuffer<Metadata> metaBuffer : register(b0, space0);

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
    accumulation_data[x + y * image_width] += pixel_color;

    float4 accumulated_color = accumulation_data[x + y * image_width];
    accumulated_color = accumulated_color / frame_index;

    accumulated_color = clamp(accumulated_color, 0.0f, 1.0f);
    image_data[x + y * config_.image_width] = ConvertToRGBA(accumulated_color);
}

