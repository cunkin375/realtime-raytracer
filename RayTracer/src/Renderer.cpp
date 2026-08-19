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
    auto background = fVector4{ 0.f, 0.f, 0.f, 1.f };
    if (scene.spheres.size() == 0)
        return background;

    const Sphere *closest_sphere = nullptr;
    f32 hit_distance = std::numeric_limits<f32>::max();

    for (const Sphere &sphere : scene.spheres)
    {
        fVector3 origin = ray.origin - sphere.position;

        f32 a = ray.direction.DotProduct(ray.direction);
        f32 b = 2.f * origin.DotProduct(ray.direction);
        f32 c = origin.MagnitudeSquared() - sphere.radius * sphere.radius;

        f32 discriminant = b * b - 4.f * a * c;

        if (discriminant < 0.0f)
            continue;

        f32 t[] = { (-b - std::sqrt(discriminant)) / (2.f * a), (-b + std::sqrt(discriminant)) / (2.f * a) };

        f32 closest_t = std::min(t[0], t[1]);

        if (closest_t < hit_distance)
        {
            hit_distance = closest_t;
            closest_sphere = &sphere;
        }
    }

    if (closest_sphere == nullptr)
        return background;

    fVector3 origin = ray.origin - closest_sphere->position;
    fVector3 hit_position = origin + ray.direction * hit_distance;
    fColor normal = fVector3::Normalize(hit_position);

    // auto light_direction = fVector3::Normalize(fVector3{-1, -1, -1});

    auto light_direction = fVector3::Normalize(light_direction_);

    f32 intensity = std::max(fVector3::DotProduct(normal, -light_direction), 0.0f);

    auto sphere_color = closest_sphere->albedo * intensity;

    return fVector4{ sphere_color.r, sphere_color.g, sphere_color.b, 1.0f };
}
