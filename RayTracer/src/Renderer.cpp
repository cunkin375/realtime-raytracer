#include "Renderer.hpp"

#include "RayTracing/ImageColor.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

// Public Methods

Renderer::Renderer()
{
    thread_count_ = std::thread::hardware_concurrency();
    threads_.reserve(thread_count_);
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

    delete[] image_data_;
    image_data_ = new u32[width * height];
}

void Renderer::Render()
{
    f32 aspect_ratio = static_cast<f32>(final_image_->GetWidth()) / final_image_->GetHeight();

    for (auto thread{ 0zu }; thread < thread_count_; ++thread)
    {
        threads_.emplace_back(
            [&, thread]()
            {
                for (auto y{ thread }; y < final_image_->GetHeight(); y += thread_count_)
                {
                    for (auto x{ 0zu }; x < final_image_->GetWidth(); ++x)
                    {
                        auto pixel_coordinate =
                            fVector2{ static_cast<f32>(x) / static_cast<f32>(final_image_->GetWidth()),
                                      static_cast<f32>(y) / static_cast<f32>(final_image_->GetHeight()) };

                        pixel_coordinate = pixel_coordinate * 2.f - 1.f;

                        pixel_coordinate.x *= aspect_ratio;

                        fVector4 pixel_color = PerPixel(pixel_coordinate);
                        pixel_color.Clamp(fVector4{ 0.0f }, fVector4{ 1.0f });

                        image_data_[x + y * final_image_->GetWidth()] =
                            ImageColor::ConvertToRGBA(pixel_color);
                    }
                }
            });
    }

    threads_.clear();

    final_image_->SetData(image_data_);
}

// Private Methods

fVector4 Renderer::PerPixel(fVector2 coord)
{
    f32 radius = 0.5f;

    auto ray_origin = fVector3{ 0, 0, 1.5f };
    auto ray_direction = fVector3{ coord.x, coord.y, -1 };

    f32 a = ray_direction.DotProduct(ray_direction);
    f32 b = 2.f * ray_origin.DotProduct(ray_direction);
    f32 c = ray_origin.MagnitudeSquared() - radius * radius;

    f32 discriminant = b * b - 4.f * a * c;

    if (discriminant < 0.0f)
    {
        // This currently does not work as intended
        // u32 color = Math::Rand::GenerateRandomNumber<u32>();
        // return fVector4{ static_cast<f32>(color & 0xFF), static_cast<f32>((color >> 8) & 0xFF),
        //                  static_cast<f32>((color >> 16) & 0xFF), 255.f };
        return fVector4{ 0.f, 0.f, 0.f, 0.f };
    }

    float t[] = { (-b - std::sqrt(discriminant)) / (2.f * a), (-b + std::sqrt(discriminant)) / (2.f * a) };

    fVector3 hit_position = ray_origin + ray_direction * t[0];
    fColor normal = fVector3::Normalize(hit_position);

    // auto light_direction = fVector3::Normalize(fVector3{-1, -1, -1});

    auto light_direction = fVector3::Normalize(light_direction_);

    f32 intensity = std::max(fVector3::DotProduct(normal, -light_direction), 0.0f);

    // auto sphere_color = normal * 0.5 + 0.5;

    auto sphere_color = sphere_colors_ * intensity;

    return fVector4{ sphere_color.r, sphere_color.g, sphere_color.b, static_cast<f32>(0xFF) };
}
