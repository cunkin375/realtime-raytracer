#include "Common.hlsl"

#define PI 3.141592653589793238462643383279f
#define LIGHT_CONSTANT 299792458.f
#define ROTATE_Y(a) mat3(1, 0, 0, 0, cos(a), sin(a), 0, -sin(a), cos(a))
#define ROTATE_Z(a) mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)

struct BlackHole
{
    float2 position;
    float mass;
    float event_horizon;
};

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