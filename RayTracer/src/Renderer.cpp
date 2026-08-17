#include "Renderer.hpp"

#include "Math/Random.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

// Public Methods

void Renderer::OnResize(u32 width, u32 height)
{
    if (final_image_)
    {
        if (final_image_->GetWidth() == width && final_image_->GetHeight() == height) return;
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

    for (auto y{ 0zu }; y < final_image_->GetHeight(); ++y)
    {
        for (auto x{ 0zu }; x < final_image_->GetWidth(); ++x)
        {
            auto pixel_coordinate =
                fVector2{ static_cast<f32>(x) / static_cast<f32>(final_image_->GetWidth()),
                          static_cast<f32>(y) / static_cast<f32>(final_image_->GetHeight()) };

            pixel_coordinate = pixel_coordinate * 2.f - 1.f;

            pixel_coordinate.x *= aspect_ratio;

            image_data_[x + y * final_image_->GetWidth()] = PerPixel(pixel_coordinate);
        }
    }

    final_image_->SetData(image_data_);
}

// Private Methods

u32 Renderer::PerPixel(fVector2 coord)
{
    f32 radius = 0.5f;

    auto ray_origin = fVector3{ 0.f, 0.f, 2.f };
    auto ray_direction = fVector3{ coord.x, coord.y, -1.f };

    f32 a = ray_direction.DotProduct(ray_direction);
    f32 b = 2.f * ray_origin.DotProduct(ray_direction);
    f32 c = ray_origin.MagnitudeSquared() - radius * radius;

    f32 discriminant = b * b - 4.f * a * c;

    if (discriminant >= 0.0f)
    {
        auto t = new float[]{ (-b - std::sqrt(discriminant)) / (2.f * a),
                              (-b + std::sqrt(discriminant)) / (2.f * a) };

        u32 sphere_color = 0xff000000;
        fVector3 hit_position = ray_origin + ray_direction * t[0];
        fVector3 normal = hit_position;
        normal.Normalize();

        auto shift = 0;

        for (auto j{ 0zu }; j < 3; ++j)
        {
            auto bits = 8;
            u32 max_val = (1U << bits) - 1;

            f32 clamped = std::clamp(normal[j] * 0.5f + 0.5f, 0.f, 1.0f);

            u32 scaled = static_cast<u32>(clamped * max_val + 0.5f);

            sphere_color |= (scaled << shift);
            shift += bits;
        }

        return sphere_color;
    }

    u32 color = Math::Rand::GenerateRandomNumber<u32>();
    color |= 0xff000000;

    return color;
}
