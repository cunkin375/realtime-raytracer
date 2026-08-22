#version 460

struct HitRecord
{
    vec3 world_position;
    vec3 world_normal;
    float hit_distance;
    int object_index;
};

struct Ray
{
    vec3 origin;
    vec3 direction;
};

int main()
{
    vec4 pixel_color = RayGen(x, y);
    accumulation_data[x + y * config_.image_width] += pixel_color;

    vec4 accumulated_color = accumulation_data[x + y * image_width];
    accumulated_color = accumulated_color / frame_index;

    clamped_color = clamp(accumulated_color, vec4(0.0f), vec4(1.0f));
    image_data[x + y * config_.image_width] =
        ConvertToRGBA(accumulated_color);
}

vec4 RayGen(uint x, uint y)
{}

vec4 ConvertToRGBA()
{}

HitRecord TraceRay(const Ray ray)
{}

HitRecord ClosestHit(const Ray ray, float hit_distance, int object_index)
{}

HitRecord Miss(const Ray ray)
{}
