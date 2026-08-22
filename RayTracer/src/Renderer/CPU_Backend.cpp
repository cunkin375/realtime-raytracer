#include "CPU_Backend.hpp"

#include <ranges>

#include "Math/Vector.hpp"
#include "RayTracing/ImageColor.hpp"
#include "Util/Aliases.hpp"

#include "Camera.hpp"
#include "Scene.hpp"

CPU_Backend::CPU_Backend()
{
#if MULTI_THREAD
    thread_count_ = std::thread::hardware_concurrency();
    threads_.reserve(thread_count_);
#endif
}

void CPU_Backend::SetImageParameters(u32 width, u32 height, u32 frame_index, bool is_fast_random_enabled)
{
    frame_ = FrameInfo{
        .width = width, .height = height, .index = frame_index, .fast_random = is_fast_random_enabled
    };
}

void CPU_Backend::Render(const Camera &camera, const Scene &scene, u32 *image_data,
                         fVector4 *accumulation_data)
{
    active_camera_ = &camera;
    active_scene_ = &scene;

#if MULTI_THREAD
    for (auto thread{ 0zu }; thread < thread_count_; ++thread)
    {
        threads_.emplace_back(
            [&, thread]()
            {
                for (auto y{ thread }; y < frame_.height; y += thread_count_)
                {
                    for (auto x{ 0zu }; x < frame_.width; ++x)
                    {
                        fVector4 pixel_color = RayGen(x, y);
                        accumulation_data[x + y * frame_.width] += pixel_color;

                        fVector4 accumulated_color = accumulation_data[x + y * frame_.width];
                        accumulated_color /= static_cast<f32>(frame_.index);

                        accumulated_color.Clamp(fVector4{ 0.0f }, fVector4{ 1.0f });
                        image_data[x + y * frame_.width] = ImageColor::ConvertToRGBA(accumulated_color);
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
}

// Private Methods

// NOTE: When transferring to Vulkan, GL_LaunchID will refer coordinates of a pixel
fVector4 CPU_Backend::RayGen(u32 x, u32 y)
{
    const auto &position = active_camera_->GetPosition();
    const auto &cached_direction = active_camera_->GetRayDirections().at(x + y * frame_.width);

    Ray ray;
    ray.origin = fVector3{ position.x, position.y, position.z };
    ray.direction = fVector3{ cached_direction.x, cached_direction.y, cached_direction.z };

    // a ray's light starts at 0 and accumulates light from light sources
    auto light = fColor{ 0 };
    // it fully contributes collor across all channels, but diminishes as it hits materials that absorb color
    auto color_contribution = fVector3{ 1.f };
    // together, light and color_contribution are implemented to support shadows through difussed lighting

    std::uint32_t seed = x + y * frame_.width;
    seed *= frame_.index;

    auto bounces{ 8zu };
    for (auto i{ 0zu }; i < bounces; ++i)
    {
        seed += i;

        auto hit_record = TraceRay(ray);
        if (hit_record.hit_distance < 0.f)
        {
            // NOTE: background contributes to rendered light
            light += active_scene_->background * color_contribution;
            break;
        }

        // grab the sphere and its material
        const auto &closest_sphere = active_scene_->spheres.at(hit_record.object_index);
        const auto &material = active_scene_->materials.at(closest_sphere.material_index);

        light += material.GetEmmission();
        color_contribution *= material.albedo;

        // ray is moved slightly outward to avoid being initiated from inside a surface
        ray.origin = hit_record.world_position + hit_record.world_normal * 0.0001f;

        auto random_vector = [&seed](bool fast_random)
        {
            return (fast_random ? fVector3::FastUnitSphereVector(seed)
                                : fVector3::GenerateRandomUnitVector());
        };

        if (material.metallic)
        {
            ray.direction = fVector3::ReflectFromSurfaceUnit(
                ray.direction, fVector3::Normalize(hit_record.world_normal
                                                   + material.roughness * random_vector(frame_.fast_random)));
        }
        else
        {
            ray.direction = fVector3::Normalize(hit_record.world_normal
                                                + material.roughness * random_vector(frame_.fast_random));
        }
    }

    return fVector4{ light.r, light.g, light.b, 1.f };
}

CPU_Backend::HitRecord CPU_Backend::TraceRay(const Ray &ray)
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

        if (discriminant < 0.f)
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

CPU_Backend::HitRecord CPU_Backend::ClosestHit(const Ray &ray, f32 hit_distance, i32 object_index)
{
    auto hit_record = HitRecord{ .hit_distance = hit_distance, .object_index = object_index };
    const auto &closest_sphere = active_scene_->spheres.at(object_index);

    fVector3 origin = ray.origin - closest_sphere.position;
    hit_record.world_position = origin + ray.direction * hit_distance;
    hit_record.world_normal = fVector3::Normalize(hit_record.world_position);

    hit_record.world_position += closest_sphere.position;
    return hit_record;
}

CPU_Backend::HitRecord CPU_Backend::Miss(const Ray &ray) { return { .hit_distance = -1 }; }

// TODO: come back to these
// Renderer::HitRecord Renderer::AnyHit(const Ray &ray, f32 hit_distance, u32 object_index) {}
// Renderer::HitRecord Renderer::Intersection(const Ray &ray, f32 hit_distance, u32 object_index) {}
