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
private:
    std::vector<std::jthread> threads_{};
    std::shared_ptr<Walnut::Image> final_image_{};
    u32 *image_data_ = nullptr;
    fVector3 light_direction_{ -1, -1, -1 };
    u32 thread_count_{ 0 };

public:
    Renderer();

    void Render(const Camera &camera, const Scene &scene);
    void OnResize(u32 width, u32 height);

    std::shared_ptr<Walnut::Image> GetFinalImage() const noexcept { return final_image_; }

    const fVector3 &GetLightDirection() const noexcept { return light_direction_; }

    void SetX(f32 in_val) { light_direction_.x = in_val; }
    void SetY(f32 in_val) { light_direction_.y = in_val; }
    void SetZ(f32 in_val) { light_direction_.z = in_val; }

private:
    fVector4 TraceRay(const Ray &ray, const Scene &scene);
};
