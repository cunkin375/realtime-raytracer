#pragma once

#include <thread>

#include "Camera.hpp"
#include "Scene.hpp"

#include "Math/Vector.hpp"
#include "RayTracing/Ray.hpp"
#include "Util/Aliases.hpp"

class CPU_Backend
{
private:
    struct HitRecord
    {
        fVector3 world_position;
        fVector3 world_normal;
        f32 hit_distance;
        i32 object_index;
    };

    struct Settings
    {
        u32 image_width;
        u32 image_height;
        u32 frame_index;
        bool fast_random;
    };

private:
    std::vector<std::jthread> threads_;
    u32 thread_count_{ 0 };

    Settings config_;

    const Scene *active_scene_{ nullptr };
    const Camera *active_camera_{ nullptr };

public:
    CPU_Backend();

    void Render(const Camera &camera, const Scene &scene, u32 *image_data, fVector4 *accumulation_data);

    void SetImageParameters(u32 width, u32 height, u32 frame_index_, bool is_fast_random_enabled);

private:
    // NOTE: All of these function resemble shaders in a Vulkan ray tracing pipeline
    fVector4 RayGen(u32 x, u32 y);

    HitRecord TraceRay(const Ray &ray);
    HitRecord ClosestHit(const Ray &ray, f32 hit_distance, i32 object_index);
    HitRecord Miss(const Ray &ray);

    // TODO: come back to these
    HitRecord AnyHit(const Ray &ray, f32 hit_distance, u32 object_index);
    HitRecord Intersection(const Ray &ray, f32 hit_distance, i32 object_index);
};
