#ifndef CAMERA_TPP_INSIDE_
#error "Do not include Camera.tpp directly; include Camera.hpp instead."
#endif

#include <iostream>
#include <thread>
#include <vector>

#include "ImageColor.hpp"

template <typename WorldType>
void Camera::RenderPass(const WorldType &world)
{
    Camera::InitializePass();

    std::cout << "P3\n" << image_width << " " << image_height_ << "\n255\n";

    for (auto j{0zu}; j < image_height_; ++j)
    {
        for (auto i{0zu}; i < image_width; ++i)
        {
            auto pixel_center = pixel_00_location_ + (i * pixel_delta_u_) + (j * pixel_delta_v_);
            auto ray_direction = pixel_center - camera_center_;
            auto ray = dRay{camera_center_, ray_direction};
            dColor pixel_color = RayToColor(ray, max_depth, world);
            ImageColor::WriteColor(std::cout, pixel_color);
        }
    }
}

template <typename WorldType>
void Camera::AntialiasingRenderPass(const WorldType &world)
{
    InitializePass();

    // all pixels are stored here
    auto framebuffer = std::vector<dColor>(image_width * image_height_);
    const auto thread_count = std::thread::hardware_concurrency();

    auto threads = std::vector<std::jthread>{};
    threads.reserve(thread_count);

    // because there are no data races when rendering, trace in parallel
    // each thread starts from a row index from (0, thread_count-1) and wors on the next N * thread_count row
    for (auto t{0zu}; t < thread_count; ++t)
    {
        threads.emplace_back(
            [&, t]()
            {
                for (auto j{t}; j < image_height_; j += thread_count)
                {
                    for (auto i{0zu}; i < image_width; ++i)
                    {
                        auto pixel_color = dColor{};
                        for (auto sample{0zu}; sample < samples_per_pixel; ++sample)
                        {
                            auto ray = GetRay(i, j);
                            pixel_color += RayToColor(ray, max_depth, world);
                        }
                        framebuffer[j * image_width + i] = pixel_samples_scale_ * pixel_color;
                    }
                }
            });
    }

    // NOTE: threads must end before writing to ppm
    threads.clear();

    // write to the ppm file
    std::cout << "P3\n" << image_width << " " << image_height_ << "\n255\n";
    for (const auto &color : framebuffer)
    {
        ImageColor::WriteColor(std::cout, color);
    }
}

template <typename WorldType>
dColor Camera::RayToColor(const dRay &ray, std::size_t depth, const WorldType &world) const
{
    if (depth <= 0) return dColor{0, 0, 0};

    // if we hit something, render it
    if (auto record = world.Hit(ray, dInterval{0.001, dInterval::PositiveInfinity()}); record != std::nullopt)
    {

        auto scatter_result = std::visit([&](const auto &material) { return material.Scatter(ray, *record); },
                                         *record->material_view);

        // this returns a diffused color
        if (scatter_result)
            return scatter_result->attenuation * RayToColor(scatter_result->scattered_ray, depth - 1, world);
    }

    // otherwise, just color the background
    dVector3 unit_direction = dVector3::Normalize(ray.direction);
    f64 alpha = 0.5 * (unit_direction.y + 1.0);

    // blend from blue up top to white at the bottom
    return (1.0 - alpha) * /* white */ dColor{1.0, 1.0, 1.0} + alpha * /* light blue */ dColor{0.5, 0.7, 1.0};
}
