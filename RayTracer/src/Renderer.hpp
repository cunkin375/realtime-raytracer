#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "Walnut/Image.h"

#include "Scene.hpp"

#include "Camera.hpp"
#include "Math/Vector.hpp"
#include "RayTracing/Ray.hpp"
#include "Util/Aliases.hpp"

class Renderer
{
public:
    struct Settings
    {
        bool accumulate{ true };
        bool fast_random{ true };
    };

private:
    struct HitRecord
    {
        fVector3 world_position;
        fVector3 world_normal;
        f32 hit_distance;
        i32 object_index;
    };

private:
    std::vector<std::jthread> threads_{};

    std::vector<u32> image_horizontal_iterator_;
    std::vector<u32> image_vertical_iterator_;

    std::shared_ptr<Walnut::Image> final_image_{};

    Settings settings_;

    u32 *image_data_{ nullptr };
    fVector4 *accumulation_data_{ nullptr };

    const Scene *active_scene_{ nullptr };
    const Camera *active_camera_{ nullptr };

    u32 thread_count_{ 0 };

    u32 frame_index_{ 1 };

public:
    Renderer();

    void Render(const Camera &camera, const Scene &scene);
    void OnResize(u32 width, u32 height);

    std::shared_ptr<Walnut::Image> GetFinalImage() const noexcept { return final_image_; }

    void ResetFrameIndex() { frame_index_ = 1; }

    Settings &GetSettings() { return settings_; }

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
