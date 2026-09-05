#include "Renderer.hpp"

#include <cstring>

#include <glm/geometric.hpp>

#include "Math/Vector.hpp"
#include "ImageColor.hpp"
#include "Ray.hpp"
#include "Scene.hpp"
#include "Util/Aliases.hpp"
#include "Util/Log.hpp"

// Public Methods

Renderer::~Renderer()
{
    delete[] image_data_;
    delete[] accumulation_data_;
}

void Renderer::SetBackend(i32 incoming_backend)
{
    switch (incoming_backend)
    {
        case 0: active_backend_ = Backend::CPU; break;
        case 1: active_backend_ = gpu_.ValidState() ? Backend::GPU : Backend::CPU; break;
        default: Log::Fatal("Unknown backend reached Renderer!"); std::exit(-1);
    }
}

void Renderer::OnUpdate(f32 timestamp)
{
    if (!gpu_.ValidState())
        active_backend_ = Backend::CPU;
}

void Renderer::OnResize(u32 width, u32 height)
{
    if (final_image_)
    {
        if (final_image_->GetWidth() == width && final_image_->GetHeight() == height)
            return;
        final_image_->Resize(width, height);
    }
    else
    {
        final_image_ = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
    }

    ResetFrameIndex();

    delete[] image_data_;
    image_data_ = new u32[width * height];

    delete[] accumulation_data_;
    accumulation_data_ = new fVector4[width * height];
}

void Renderer::Render(const Camera &camera, const Scene &scene)
{
    if (frame_index_ == 1)
        std::memset(accumulation_data_, 0,
                    final_image_->GetWidth() * final_image_->GetHeight() * sizeof(fVector4));

    // these Render methods write into image_data_ and accumulation_data_
    switch (active_backend_)
    {
        case Backend::CPU:
            cpu_.SetImageParameters(final_image_->GetWidth(), final_image_->GetHeight(), frame_index_,
                                    settings_.fast_random);
            cpu_.Render(camera, scene, image_data_, accumulation_data_);
            final_image_->SetData(image_data_);
            break;
        case Backend::GPU:
            gpu_.SetImageParameters(final_image_->GetWidth(), final_image_->GetHeight(), frame_index_);
            gpu_.Render(camera, scene);
            break;
        default: Log::Fatal("Unknown backend reached Renderer!"); std::exit(-1);
    }

    if (settings_.accumulate)
        frame_index_++;
    else
        frame_index_ = 1;
}

VkDescriptorSet Renderer::GetDescriptorSet()
{
    switch (active_backend_)
    {
        case Backend::CPU: return final_image_->GetDescriptorSet();
        case Backend::GPU: return gpu_.GetDescriptorSet();
        default: Log::Fatal("Unknown backend reached Renderer!"); std::exit(-1);
    }
}
