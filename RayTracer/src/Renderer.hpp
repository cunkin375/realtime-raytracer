#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "Walnut/Image.h"

#include "Math/Vector.hpp"

#include "Util/Aliases.hpp"

class Renderer
{
private:
    std::vector<std::jthread> threads_{};
    std::shared_ptr<Walnut::Image> final_image_{};
    u32 *image_data_ = nullptr;
    u32 thread_count_{ 0 };

public:
    Renderer();

    void Render();
    void OnResize(u32 width, u32 height);

    std::shared_ptr<Walnut::Image> GetFinalImage() const noexcept { return final_image_; }

private:
    fVector4 PerPixel(fVector2 coord);
};
