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

    enum class Backend
    {
        CPU,
        GPU
    };

private:
    Settings settings_;
    Backend active_backend_{ Backend::GPU };
    CPU_Backend cpu_;
    GPU_Backend gpu_;

    std::shared_ptr<Walnut::Image> final_image_{};

    u32 *image_data_{ nullptr };
    fVector4 *accumulation_data_{ nullptr };

    u32 frame_index_{ 1 };

public:
    Renderer() = default;

    void OnUpdate(f32 timestamp);

    void Render(const Camera &camera, const Scene &scene);
    void OnResize(u32 width, u32 height);

    void SetBackend(Renderer::Backend new_backend) { active_backend_ = new_backend; }

    std::shared_ptr<Walnut::Image> GetFinalImage() const noexcept { return final_image_; }

    void ResetFrameIndex() { frame_index_ = 1; }

    Settings &GetSettings() { return settings_; }
};
