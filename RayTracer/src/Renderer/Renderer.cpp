#include "Renderer.hpp"

// Public Methods

void Renderer::OnResize(u32 width, u32 height)
{
    if (width == 0 || height == 0)
        return;

    if (width == width_ && height == height_)
        return;

    width_  = width;
    height_ = height;

    ResetFrameIndex();
}

void Renderer::Render(const Camera &camera, const Scene &scene)
{
    if (width_ == 0 || height_ == 0)
        return;

    gpu_.SetImageParameters(width_, height_, frame_index_);
    gpu_.Render(camera, scene);

    if (settings_.accumulate)
        frame_index_++;
    else
        frame_index_ = 1;
}

VkDescriptorSet Renderer::GetDescriptorSet() { return gpu_.GetDescriptorSet(); }
