#define PI 3.141592653589793238462643383279f
#define LIGHT_CONSTANT 299792458.f
#define ROTATE_Y(a) mat3(1, 0, 0, 0, cos(a), sin(a), 0, -sin(a), cos(a))
#define ROTATE_Z(a) mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)

struct Metadata
{
    float3 camera_position;
    float  _pad0;
    float4x4 camera_inverse_view;
    float4x4 camera_inverse_projection;
    float3 background;
    float  image_width;
    float  image_height;
    float  frame_index;
    uint   num_spheres;
};

struct Sphere
{
    float3 position;
    float radius;
    int material_index;
    int3 _pad;
};

struct Material
{
    float3 albedo;
    float roughness;
    float3 emission_color;
    float emission_power;
    bool metallic;
    float3 _pad;

    float3 GetEmission() { return emission_color * emission_power; }
};

struct HitRecord
{
    float3 world_position;
    float3 world_normal;
    float hit_distance;
    int object_index;
};

struct Ray
{
    float3 origin;
    float3 direction;
};

struct BlackHole
{
    float2 position;
    float mass;
    float event_horizon;
};

// Vulkan Bindings
// vk::binding(binding_number, descriptor_set_number) or binding(binding_number)
[[vk::binding(0, 0)]] ConstantBuffer<Metadata> meta_buffer : register(b0, space0);
[[vk::binding(1, 0)]] StructuredBuffer<Sphere> spheres : register(t0, space0);
[[vk::binding(2, 0)]] StructuredBuffer<Material> materials : register(t1, space0);
[[vk::binding(3, 0)]] RWStructuredBuffer<float4> accumulation_data : register(u0, space0);
[[vk::binding(4, 0)]] RWTexture2D<float4> image_data : register(u1, space0);


// Returns a random seed between 0 and 1
// source: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint PCG_Hash(uint input)
{
    uint state = input * 747796405u + 289133643u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803747u;
    return (word >> 22u) ^ word;
}

HitRecord ClosestHit(const Ray ray, float hit_distance, int object_index)
{
    HitRecord record;
    return record;
}

HitRecord Miss(const Ray ray)
{
    HitRecord record;
    record.hit_distance = -1;
    return record;
}

HitRecord TraceRay(const Ray ray)
{
    HitRecord record;
    return record;
}

float3 CalculateRayDirection(float x, float y)
{
    float2 coord = float2(x / meta_buffer.image_width, y / meta_buffer.image_height);
    coord = coord * 2.f - 1.f;

    float4 target = mul(meta_buffer.camera_inverse_projection, float4(coord.x, coord.y, 1.f, 1.f));

    // convert projected position to 3D (divide by w), then transform by inverse view
    float3 ndc = normalize(target.xyz / target.w);
    float4 world_direction = mul(meta_buffer.camera_inverse_view, float4(ndc, 0.f));

    return normalize(world_direction.xyz);
}

float4 RayGen(uint x, uint y)
{
    return float4(0.f, 0.f, 0.f, 0.f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float4 error_magenta = float4(1, 0, 1, 1);
    const float gravity = 3;

    uint x = id.x;
    uint y = id.y;

    BlackHole black_hole;
    black_hole.mass = 5;
    black_hole.event_horizon = (2.0f * gravity * black_hole.mass) / (LIGHT_CONSTANT * LIGHT_CONSTANT);
    black_hole.position = float2(0.f, 0.f);

    float4 pixel_color = RayGen(x, y);
    image_data[uint2(x, y)] = error_magenta;
}