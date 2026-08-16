#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Hittable.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

class Camera
{
public:
    f64 aspect_ratio{16.0 / 9.0};
    std::size_t image_width{400};
    std::size_t samples_per_pixel{10};
    std::size_t max_depth{10};

    f64 vertical_fov{90};
    dPoint3 look_from{0, 0, 0};
    dPoint3 look_at{0, 0, -1};
    dVector3 vertical_up{0, 1, 0};

    f64 defocus_angle{0};
    f64 focus_distance{10};

private:
    std::size_t image_height_{};
    f64 pixel_samples_scale_{};
    dPoint3 camera_center_{};
    dPoint3 pixel_00_location_{};
    dVector3 pixel_delta_u_;
    dVector3 pixel_delta_v_;
    dVector3 u, v, w;
    dVector3 defocus_disk_u;
    dVector3 defocus_disk_v;

public:
    Camera(f64 _aspect_ratio, std::size_t _image_width);
    Camera(f64 _aspect_ratio, std::size_t _image_width, std::size_t _samples_per_pixel);
    Camera(f64 _aspect_ratio, std::size_t _image_width, std::size_t _samples_per_pixel, f64 _vertical_fov);

    template <typename WorldType>
    void RenderPass(const WorldType &world);
    template <typename WorldType>
    void AntialiasingRenderPass(const WorldType &world);

private:
    dRay GetRay(std::size_t i, std::size_t j) const;
    dVector3 SampleSquare() const;
    dPoint3 DefocusDiskSample() const;

    template <typename WorldType>
    dColor RayToColor(const dRay &ray, std::size_t depth, const WorldType &world) const;

    void InitializePass();
};

// ==== template methods
// =====================================================================================================
// NOTE: this slows down compilation, it can be moved to Camera.tpp if it becomes too slow, but will break
// linting
#define CAMERA_TPP_INSIDE_

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

#undef CAMERA_TPP_INSIDE_

#endif // CAMERA_HPP
