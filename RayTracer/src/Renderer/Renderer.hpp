#pragma once

#include "Camera.hpp"
#include "GPU_Backend.hpp"
#include "Scene.hpp"
#include "Util/Aliases.hpp"

class Renderer
{
public:
    struct Settings
    {
        bool accumulate{ true };
    };

private:
    Settings    settings_;
    GPU_Backend gpu_;

    u32 width_{ 0 };
    u32 height_{ 0 };
    u32 frame_index_{ 1 };

public:
    Renderer() = default;
    ~Renderer() = default;

    void OnUpdate([[maybe_unused]] f32 timestamp) {}
    void OnResize(u32 width, u32 height);

    void Render(const Camera &camera, const Scene &scene);

    VkDescriptorSet GetDescriptorSet();

    void ResetFrameIndex() { frame_index_ = 1; }

    Settings &GetSettings() { return settings_; }
};
