// std140 layout requires pads float3 to float4 size
// pad vars added to reflect GPU_Metada struct in GPU_Backend.hpp
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

// Vulkan Bindings
// vk::binding(binding_number, descriptor_set_number) or binding(binding_number)
[[vk::binding(0, 0)]] ConstantBuffer<Metadata> meta_buffer : register(b0, space0);
[[vk::binding(1, 0)]] StructuredBuffer<Sphere> spheres : register(t0, space0);
[[vk::binding(2, 0)]] StructuredBuffer<Material> materials : register(t1, space0);
[[vk::binding(3, 0)]] RWStructuredBuffer<float4> accumulation_data : register(u0, space0);
[[vk::binding(4, 0)]] RWTexture2D<float4> image_data : register(u1, space0);
