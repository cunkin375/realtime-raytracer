#include "Renderer.hpp"

#include <limits>
#include <ranges>

#include "RayTracing/ImageColor.hpp"
#include "RayTracing/Ray.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

// Public Methods

Renderer::Renderer()
{
#if MULTI_THREAD
    thread_count_ = std::thread::hardware_concurrency();
    threads_.reserve(thread_count_);
#endif // MULTI_THREAD
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
    active_scene_ = &scene;
    active_camera_ = &camera;

#if MULTI_THREAD
    for (auto thread{ 0zu }; thread < thread_count_; ++thread)
    {
        threads_.emplace_back(
            [&, thread]()
            {
                for (auto y{ thread }; y < final_image_->GetHeight(); y += thread_count_)
                {
                    for (auto x{ 0zu }; x < final_image_->GetWidth(); ++x)
                    {
                        fVector4 pixel_color = RayGen(x, y);
                        pixel_color.Clamp(fVector4{ 0.0f }, fVector4{ 1.0f });
                        image_data_[x + y * final_image_->GetWidth()] =
                            ImageColor::ConvertToRGBA(pixel_color);
                    }
                }
            });
    }
    // threads must join before uploading to vulkan buffer
    threads_.clear();
#endif

#ifndef MULTI_THREAD
    for (auto y{ 0zu }; y < final_image_->GetHeight(); ++y)
    {
        for (auto x{ 0zu }; x < final_image_->GetWidth(); ++x)
        {
            fVector4 pixel_color = RayGen(x, y);
            pixel_color.Clamp(fVector4{ 0.0f }, fVector4{ 1.0f });
            image_data_[x + y * final_image_->GetWidth()] = ImageColor::ConvertToRGBA(pixel_color);
        }
    }
#endif

    final_image_->SetData(image_data_);
}

// Private Methods

// NOTE: When transferring to Vulkan, GL_LaunchID will refer coordinates of a pixel
fVector4 Renderer::RayGen(u32 x, u32 y)
{
    const auto &position = active_camera_->GetPosition();
    const auto &cached_direction = active_camera_->GetRayDirections().at(x + y * final_image_->GetWidth());

    Ray ray;
    ray.origin = fVector3{ position.x, position.y, position.z };
    ray.direction = fVector3{ cached_direction.x, cached_direction.y, cached_direction.z };

    auto color = fColor{ 0 };

    f32 multiplier = 1.f;

    auto bounces{ 2zu };
    for (auto i{ 0zu }; i < bounces; ++i)
    {
        auto hit_record = TraceRay(ray);
        if (hit_record.hit_distance < 0.f)
        {
            auto background = fVector3{ 0.f, 0.f, 0.f };
            color += background * multiplier;
            break;
        }

        fVector3 light_direction_normal = fVector3::Normalize(light_direction_);
        f32 light_intensity =
            std::max(fVector3::DotProduct(hit_record.world_normal, -light_direction_normal), 0.f); // behaves like a cos function

        const auto &closest_sphere = active_scene_->spheres[hit_record.object_index];

        auto sphere_color = closest_sphere.albedo * light_intensity;
        color += sphere_color * multiplier;

        multiplier *= 0.7f;

        // ray is moved slightly outward to avoid being initiated from inside a surface
        ray.origin = hit_record.world_position + hit_record.world_normal * 0.0001f;
        ray.direction = fVector3::ReflectFromSurfaceNormal(ray.direction, hit_record.world_normal);
    }

    return fVector4{ color.r, color.g, color.b, 1.f };
}

Renderer::HitRecord Renderer::TraceRay(const Ray &ray)
{
    i32 closest_sphere_index = -1;
    f32 hit_distance = std::numeric_limits<f32>::max();

    for (auto [index, sphere] : active_scene_->spheres | std::views::enumerate)
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

        if (closest_t > 0.0f && closest_t < hit_distance)
        {
            hit_distance = closest_t;
            closest_sphere_index = static_cast<i32>(index);
        }
    }

    if (closest_sphere_index == -1)
        return Miss(ray);

    return ClosestHit(ray, hit_distance, closest_sphere_index);
}

Renderer::HitRecord Renderer::ClosestHit(const Ray &ray, f32 hit_distance, i32 object_index)
{
    auto hit_record = Renderer::HitRecord{ .hit_distance = hit_distance, .object_index = object_index };
    const auto &closest_sphere = active_scene_->spheres.at(object_index);

    fVector3 origin = ray.origin - closest_sphere.position;
    hit_record.world_position = origin + ray.direction * hit_distance;
    hit_record.world_normal = fVector3::Normalize(hit_record.world_position);

    hit_record.world_position += closest_sphere.position;
    return hit_record;
}

Renderer::HitRecord Renderer::Miss(const Ray &ray) { return { .hit_distance = -1 }; }

// TODO: come back to these
// Renderer::HitRecord Renderer::AnyHit(const Ray &ray, f32 hit_distance, u32 object_index) {}
// Renderer::HitRecord Renderer::Intersection(const Ray &ray, f32 hit_distance, u32 object_index) {}
