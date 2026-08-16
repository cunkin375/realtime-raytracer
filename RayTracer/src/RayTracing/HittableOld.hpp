#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "Math/Interval.hpp"
#include "Math/Vector.hpp"
#include "Ray.hpp"
#include "Util/Aliases.hpp"
#include "Util/Log.hpp"
namespace Old
{

namespace [[deprecated("Use Hittable.hpp instead of HittableOld.hpp!!!")]] Hittable
{

    struct HitRecord
    {
    public:
        dPoint3 end_point;
        dVector3 normal;
        f64 distance;
        bool front_face;

    public:
        /* Here, the side of a hittable surface from the persepective of the ray is decided at geometry time
         */
        void SetFrontfaceNormal(const dRay &ray, const dVector3 &outward_normal)
        {
            front_face = (dVector3::DotProduct(ray.direction, outward_normal) < 0);
            normal = front_face ? outward_normal : -outward_normal;
        }
    };

    struct Hittable
    {
    private:
        void *data_pointer_ = nullptr;

        std::optional<HitRecord> (*hit_function_pointer_)(const void *, const dRay &ray,
                                                          Math::Interval<f64> ray_interval) = nullptr;
        void *(*copy_function_pointer_)(const void *) = nullptr;
        void (*destroy_function_pointer_)(void *) = nullptr;

    public:
        template <typename Derived>
        Hittable(Derived object)
        {
            data_pointer_ = new Derived(std::move(object));

            hit_function_pointer_ = [](const void *pointer, const dRay &ray,
                                       Math::Interval<f64> ray_interval) -> std::optional<HitRecord>
            { return static_cast<const Derived *>(pointer)->Hit(ray, ray_interval); };

            copy_function_pointer_ = [](const void *pointer) -> void *
            { return new Derived(*static_cast<const Derived *>(pointer)); };

            destroy_function_pointer_ = [](void *pointer) { delete static_cast<Derived *>(pointer); };
        }

        Hittable(const Hittable &other)
        {
            if (other.data_pointer_)
            {
                data_pointer_ = other.copy_function_pointer_(other.data_pointer_);
                hit_function_pointer_ = other.hit_function_pointer_;
                copy_function_pointer_ = other.copy_function_pointer_;
                destroy_function_pointer_ = other.destroy_function_pointer_;
            }
        }

        Hittable &operator=(Hittable &other)
        {
            std::swap(data_pointer_, other.data_pointer_);
            std::swap(hit_function_pointer_, other.hit_function_pointer_);
            std::swap(copy_function_pointer_, other.copy_function_pointer_);
            std::swap(destroy_function_pointer_, other.destroy_function_pointer_);
            return *this;
        }

        Hittable(Hittable &&other) noexcept
            : data_pointer_{other.data_pointer_}, hit_function_pointer_{other.hit_function_pointer_},
              copy_function_pointer_{other.copy_function_pointer_},
              destroy_function_pointer_{other.destroy_function_pointer_}
        { other.data_pointer_ = nullptr; }

        ~Hittable()
        {
            if (data_pointer_ && destroy_function_pointer_) { destroy_function_pointer_(data_pointer_); }
        }

        std::optional<HitRecord> Hit(const dRay &ray, Math::Interval<f64> ray_interval) const
        {
            if (data_pointer_ && hit_function_pointer_)
            {
                return static_cast<std::optional<HitRecord>>(
                    hit_function_pointer_(data_pointer_, ray, ray_interval));
            }
            else
            {
                Log::Error("Received nullopt in Hittable::Hit!");
                return std::nullopt;
            }
        }
    };

    struct HittableList
    {
    public:
        std::vector<Hittable> objects;

    public:
        HittableList() = default;

        HittableList(Hittable object) { objects.push_back(object); }

        void Add(Hittable object) { objects.push_back(std::move(object)); }

        constexpr std::optional<HitRecord> Hit(const dRay &ray, dInterval ray_interval) const
        {
            auto return_record = HitRecord{};
            auto hit_anything = bool{false};
            auto closest_so_far = ray_interval.upper;
            for (const auto &object : objects)
            {
                if (auto temp_record = object.Hit(ray, dInterval{ray_interval.lower, closest_so_far});
                    temp_record != std::nullopt)
                {
                    hit_anything = true;
                    closest_so_far = temp_record->distance;
                    return_record = temp_record.value();
                }
            }
            return hit_anything ? std::optional{return_record} : std::nullopt;
        }
    };

} // namespace Hittable

} // namespace Old
