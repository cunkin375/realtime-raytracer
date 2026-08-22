#pragma once

#include <memory>

#include "Renderer/CPU_Backend.hpp"
#include "Renderer/GPU_Backend.hpp"

#include "Walnut/Image.h"

#include "Camera.hpp"
#include "Scene.hpp"

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

    enum class Backend
    {
        CPU,
        GPU
    };

private:
    Settings settings_;
    Backend active_backend_;
    CPU_Backend cpu_;
    GPU_Backend gpu_;

    std::shared_ptr<Walnut::Image> final_image_{};

    u32 *image_data_{ nullptr };
    fVector4 *accumulation_data_{ nullptr };

    u32 frame_index_{ 1 };

public:
    Renderer() = default;

    void Render(const Camera &camera, const Scene &scene);
    void OnResize(u32 width, u32 height);

    std::shared_ptr<Walnut::Image> GetFinalImage() const noexcept { return final_image_; }

    void ResetFrameIndex() { frame_index_ = 1; }

    Settings &GetSettings() { return settings_; }
};
