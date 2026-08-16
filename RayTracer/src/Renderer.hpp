#pragma once

#include <memory>

#include "Walnut/Image.h"

#include "Math/Vector.hpp"

#include "Util/Aliases.hpp"

class Renderer
{
private:
    std::shared_ptr<Walnut::Image> final_image_{};
    u32 *image_data_ = nullptr;

public:
    Renderer() = default;

    void Render();
    void Render(f32 aspect_ratio);
    void OnResize(u32 width, u32 height);

    std::shared_ptr<Walnut::Image> GetFinalImage() const noexcept { return final_image_; }


private:
    u32 PerPixel(fVector2 coord);
};
