#include "Renderer.hpp"

#include <cstring>
#include <limits>
#include <ranges>

#include <glm/geometric.hpp>

#include "Walnut/Random.h"

#include "Math/Vector.hpp"
#include "RayTracing/ImageColor.hpp"
#include "RayTracing/Ray.hpp"
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

    ResetFrameIndex();

    delete[] image_data_;
    image_data_ = new u32[width * height];

    delete[] accumulation_data_;
    accumulation_data_ = new fVector4[width * height];
}

void Renderer::Render(const Camera &camera, const Scene &scene)
{
    active_scene_ = &scene;
    active_camera_ = &camera;

    if (frame_index_ == 1)
        std::memset(accumulation_data_, 0,
                    final_image_->GetWidth() * final_image_->GetHeight() * sizeof(fVector4));

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
                        accumulation_data_[x + y * final_image_->GetWidth()] += pixel_color;

                        fVector4 accumulated_color = accumulation_data_[x + y * final_image_->GetWidth()];
                        accumulated_color /= static_cast<f32>(frame_index_);

                        accumulated_color.Clamp(fVector4{ 0.0f }, fVector4{ 1.0f });
                        image_data_[x + y * final_image_->GetWidth()] =
                            ImageColor::ConvertToRGBA(accumulated_color);
                    }
                }
            });
    }
    // threads must join before uploading to vulkan buffer
    threads_.clear();
#else
    for (auto y{ 0zu }; y < final_image_->GetHeight(); ++y)
    {
        for (auto x{ 0zu }; x < final_image_->GetWidth(); ++x)
        {
            fVector4 pixel_color = RayGen(x, y);
            accumulation_data_[x + y * final_image_->GetWidth()] += pixel_color;

            fVector4 accumulated_color = accumulation_data_[x + y * final_image_->GetWidth()];
            accumulated_color /= static_cast<f32>(frame_index_);

            pixel_color.Clamp(fVector4{ 0.0f }, fVector4{ 1.0f });
            image_data_[x + y * final_image_->GetWidth()] = ImageColor::ConvertToRGBA(pixel_color);
        }
    }
#endif

    final_image_->SetData(image_data_);

    if (settings_.accumulate)
        frame_index_++;
    else
        frame_index_ = 1;
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

    // a ray's light starts at 0 and accumulates light from light sources
    auto light = fColor{ 0 };
    // it fully contributes collor across all channels, but diminishes as it hits materials that absorb color
    auto color_contribution = fVector3{ 1.f };
    // together, light and color_contribution are implemented to support shadows through difussed lighting

    auto bounces{ 8zu };
    for (auto i{ 0zu }; i < bounces; ++i)
    {
        auto hit_record = TraceRay(ray);
        if (hit_record.hit_distance < 0.f)
        {
            auto background = fVector3{ 0.5f, 0.5f, 0.5f };
            light += background * color_contribution;
            break;
        }

        // fVector3 light_direction_normal = fVector3::Normalize(active_scene_->light_direction);
        // // behaves like a cos function
        // f32 light_intensity =
        //     std::max(fVector3::DotProduct(hit_record.world_normal, -light_direction_normal), 0.f);

        // grab the sphere and its material
        const auto &closest_sphere = active_scene_->spheres.at(hit_record.object_index);
        const auto &material = active_scene_->materials.at(closest_sphere.material_index);

        // light += material.albedo * color_contribution;
        color_contribution *= material.albedo;

        // ray is moved slightly outward to avoid being initiated from inside a surface
        ray.origin = hit_record.world_position + hit_record.world_normal * 0.0001f;

        // ray.direction = fVector3::ReflectFromSurface(
        //     ray.direction,
        //     hit_record.world_normal + material.roughness * fVector3::GenerateRandomVector(-0.5f, 0.5f));

        // ray.direction = fVector3::RandomUnitVectorOnHemisphere(fVector3::Normalize(hit_record.world_normal));

        // ray.direction = fVector3::Normalize(hit_record.world_normal + fVector3::GenerateRandomUnitVector());

        auto world_normal = hit_record.world_normal;
        auto direction = glm::normalize(glm::vec3(world_normal.x, world_normal.y, world_normal.z) + Walnut::Random::InUnitSphere());
        ray.direction = fVector3{direction.x, direction.y, direction.z };

    }

    return fVector4{ light.r, light.g, light.b, 1.f };
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
