#include "Renderer.hpp"

#include "RayTracing/ImageColor.hpp"
#include "RayTracing/Ray.hpp"

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

void Renderer::Render(const Camera &camera, const Scene &scene)
{
    // thread safe
    const auto &position = camera.GetPosition();
    const fVector3 ray_origin = { position.x, position.y, position.z };

    for (auto thread{ 0zu }; thread < thread_count_; ++thread)
    {
        threads_.emplace_back(
            [&, thread]()
            {
                for (auto y{ thread }; y < final_image_->GetHeight(); y += thread_count_)
                {
                    for (auto x{ 0zu }; x < final_image_->GetWidth(); ++x)
                    {
                        // auto pixel_coordinate =
                        //     fVector2{ static_cast<f32>(x) / static_cast<f32>(final_image_->GetWidth()),
                        //               static_cast<f32>(y) / static_cast<f32>(final_image_->GetHeight()) };
                        // pixel_coordinate = pixel_coordinate * 2.f - 1.f;

                        Ray ray;
                        ray.origin = ray_origin;

                        const glm::vec3 &cached_direction =
                            camera.GetRayDirections()[x + y * final_image_->GetWidth()];

                        ray.direction =
                            fVector3{ cached_direction.x, cached_direction.y, cached_direction.z };

                        fVector4 pixel_color = TraceRay(ray, scene);
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

fVector4 Renderer::TraceRay(const Ray &ray, const Scene &scene)
{
    auto background = fVector4{ 0.f, 0.f, 0.f, 0.f };
    if (scene.spheres.size() == 0)
        return fVector4{ 0.f, 0.f, 0.f, 0.f };

    const Sphere &sphere = scene.spheres.at(0);

    fVector3 origin = ray.origin - sphere.position;

    f32 a = ray.direction.DotProduct(ray.direction);
    f32 b = 2.f * origin.DotProduct(ray.direction);
    f32 c = origin.MagnitudeSquared() - sphere.radius * sphere.radius;

    f32 discriminant = b * b - 4.f * a * c;

    if (discriminant < 0.0f)
    {
        // This currently does not work as intended
        // u32 color = Math::Rand::GenerateRandomNumber<u32>();
        // return fVector4{ static_cast<f32>(color & 0xFF), static_cast<f32>((color >> 8) & 0xFF),
        //                  static_cast<f32>((color >> 16) & 0xFF), 255.f };
        return background;
    }

    f32 t[] = { (-b - std::sqrt(discriminant)) / (2.f * a), (-b + std::sqrt(discriminant)) / (2.f * a) };

    // f32 final_t = std::max(0.0f, std::min(t[0], t[1]));

    fVector3 hit_position = origin + ray.direction * t[0];
    fColor normal = fVector3::Normalize(hit_position);

    // auto light_direction = fVector3::Normalize(fVector3{-1, -1, -1});

    auto light_direction = fVector3::Normalize(light_direction_);

    f32 intensity = std::max(fVector3::DotProduct(normal, -light_direction), 0.0f);

    auto sphere_color = sphere.albedo * intensity;

    return fVector4{ sphere_color.r, sphere_color.g, sphere_color.b, 1.0f };
}
