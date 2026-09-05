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

HitRecord ClosestHit(const Ray ray, float hit_distance, int object_index)
{
    HitRecord record;
    record.hit_distance = hit_distance;
    record.object_index = object_index;

    Sphere closest_sphere = spheres[object_index];

    float3 origin = ray.origin - closest_sphere.position;
    record.world_position = origin + ray.direction * hit_distance;
    record.world_normal = normalize(record.world_position);

    record.world_position += closest_sphere.position;
    return record;
}

HitRecord Miss(const Ray ray)
{
    HitRecord record;
    record.hit_distance = -1;
    return record;
}

// solves quadratic equation to determine if a ray has hit a sphere and which solution is nearest to ray.origin
HitRecord TraceRay(const Ray ray)
{
    int closest_sphere_index = -1;
    float hit_distance = 0xFFFFFFFF;

    for (uint i = 0; i < meta_buffer.num_spheres; i++)
    {
        Sphere sphere = spheres[i];

        float3 origin = ray.origin - sphere.position;

        float a = dot(ray.direction, ray.direction);
        float b = 2.f * dot(origin, ray.direction);
        float c = dot(origin, origin) - sphere.radius * sphere.radius;

        float discriminant = b * b - 4.f * a * c;

        if (discriminant < 0.f)
            continue;

        float t[] = { (-b - sqrt(discriminant)) / (2.f * a), (-b + sqrt(discriminant)) / (2.f * a) };

        float closest_t = min(t[0], t[1]);

        if (closest_t > 0.f && closest_t < hit_distance)
        {
            hit_distance = closest_t;
            closest_sphere_index = i;
        }
    }

    if (closest_sphere_index == -1)
        return Miss(ray);

    return ClosestHit(ray, hit_distance, closest_sphere_index);

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

// Generates a random vector with sampled color information
float4 RayGen(uint x, uint y)
{
    Ray ray;
    ray.origin = meta_buffer.camera_position;
    ray.direction = CalculateRayDirection(x, y);

    // a ray's light starts at 0 and accumulates light from light sources
    float3 light = float3(0.f, 0.f, 0.f);
    // it fully contributes collor across all channels, but diminishes as it hits materials that absorb color
    float3 color_contribution = float3(1.f, 1.f, 1.f);
    // together, light and color_contribution are implemented to support shadows through difussed lighting

    uint seed = x + y * meta_buffer.image_width;
    seed ^= (uint)meta_buffer.frame_index * 719393u;

    // ray bounces should be kept low
    // the GPU does not like dynamic branching in a for loop
    uint bounces = 8;
    for (uint i = 0; i < bounces; i++)
    {
        seed += i;

        HitRecord record = TraceRay(ray);
        if (record.hit_distance < 0.f)
        {
            light += meta_buffer.background * color_contribution;
            break;
        }

        const Sphere sphere = spheres[record.object_index];
        const Material material = materials[sphere.material_index];

        light += material.GetEmission();
        color_contribution *= material.albedo;

        // avoid initiation inside a surface
        ray.origin = record.world_position + record.world_normal * 0.0001f;

        if (material.metallic)
        {
            ray.direction = reflect(
                ray.direction,
                normalize(record.world_normal + material.roughness * RandomUnitSphereVector(seed)));
        }
        else
        {
            ray.direction = normalize(
                record.world_normal + material.roughness * RandomUnitSphereVector(seed));
        }
    }

    return float4(light.r, light.g, light.b, 1.f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float4 error_magenta = float4(1, 0, 1, 1);

    uint x = id.x;
    uint y = id.y;

    // keep ray tracing to pixels within the actual image
    // prevents stray pixels to the left of the viewport
    if (x >= meta_buffer.image_width || y >= meta_buffer.image_height)
        return;

    float4 pixel_color = RayGen(x, y);
    accumulation_data[x + y * meta_buffer.image_width] += pixel_color;

    float4 accumulated_color = accumulation_data[x + y * meta_buffer.image_width];
    accumulated_color = accumulated_color / meta_buffer.frame_index;

    accumulated_color = clamp(accumulated_color, 0.f, 1.f);
    image_data[uint2(x, y)] = accumulated_color;
}


