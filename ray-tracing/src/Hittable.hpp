#pragma once

#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "AABB.hpp"
#include "Material.hpp"
#include "Ray.hpp"
#include "Sphere.hpp"

#include "Math/Interval.hpp"
#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

struct HitRecord
{
public:
    dPoint3 end_point;
    dVector3 normal;
    const Material *material_view;
    f64 distance;
    f64 u_texture_coord;
    f64 v_texture_coord;
    bool front_face;

public:
    /* Here, the side of a hittable surface from the persepective of the ray is decided at geometry time */
    void SetFrontfaceNormal(const dRay &ray, const dVector3 &outward_normal)
    {
        front_face = (dVector3::DotProduct(ray.direction, outward_normal) < 0);
        normal = front_face ? outward_normal : -outward_normal;
    }
};

struct HittableReference
{
    std::size_t pool_index;
    std::size_t object_index;
    AABB<f64> bounding_box;
};

template <typename... ShapeArgs>
struct HittableList
{
public:
    std::tuple<std::vector<ShapeArgs>...> object_pools{};
    std::size_t num_objects{0};

private:
    AABB<f64> bounding_box_;

public:
    HittableList() = default;

    template <typename Shape>
    void Add(Shape object)
    {
        std::get<std::vector<Shape>>(object_pools).push_back(std::move(object));
        bounding_box_ = AABB{bounding_box_, object.BoundingBox()};
        num_objects++;
    }

    constexpr std::optional<HitRecord> Hit(const dRay &ray, dInterval ray_interval) const
    {
        auto return_record = HitRecord{};
        auto hit_anything = bool{false};
        auto closest_so_far = ray_interval.upper;

        // fold over each <Shape> type's vector for all <... ShapeArgs>
        std::apply(
            [&](const auto &...type_pools)
            {
                ((
                     [&](const auto &pool)
                     {
                         for (const auto &object : pool)
                         {
                             if (auto temp = object.Hit(ray, dInterval{ray_interval.lower, closest_so_far}))
                             {
                                 hit_anything = true;
                                 closest_so_far = temp->distance;
                                 return_record = *temp;
                             }
                         }
                     }(type_pools)),
                 ...); // go to the next ShapeArgument
            },
            object_pools); // apply this fold to object_pools

        return hit_anything ? std::optional{return_record} : std::nullopt;
    }

    std::vector<HittableReference> BuildReferenceVector(const auto &object_pools) const
    {
        auto references = std::vector<HittableReference>{};
        auto pool_index = std::size_t{0};

        // looks nightmarish but this just makes a vector of references to each type pool
        std::apply(
            [&](const auto &...pools)
            {
                ((
                     [&](const auto &pool)
                     {
                         for (auto i{0zu}; i < pool.size(); ++i)
                         {
                             references.push_back(HittableReference{.pool_index = pool_index,
                                                                    .object_index = i,
                                                                    .bounding_box = pool[i].BoundingBox()});
                         }
                         ++pool_index;
                     }(pools)),
                 ...);
            },
            object_pools);

        return references;
    }

    AABB<f64> BoundingBox() const noexcept { return bounding_box_; }
};

using HitFunction = std::optional<HitRecord> (*)(const void *pool_pointer, std::size_t object_index,
                                                 const dRay &ray, dInterval ray_interval);

template <typename Shape>
std::optional<HitRecord> DispatchHit(const void *pool_pointer, std::size_t object_index, const dRay &ray,
                                     dInterval ray_interval)
{
    const auto &pool = *static_cast<const std::vector<Shape> *>(pool_pointer);
    return pool[object_index].Hit(ray, ray_interval);
}

template <typename... ShapeArgs>
struct HitFunctionDispatchTable
{
    std::array<const void *, sizeof...(ShapeArgs)> pool_pointers;
    std::array<HitFunction, sizeof...(ShapeArgs)> hit_functions;

    constexpr HitFunctionDispatchTable(const std::tuple<std::vector<ShapeArgs>...> &pools)
    {
        // build pool_pointers
        auto index = std::size_t{0};
        std::apply([&](const auto &...vectors)
                   { ((pool_pointers[index++] = static_cast<const void *>(&vectors)), ...); }, pools);

        // build hit_functions
        index = 0;
        ((hit_functions[index++] = &DispatchHit<ShapeArgs>), ...);
    }

    constexpr std::optional<HitRecord> Hit(const HittableReference &reference, const dRay &ray,
                                           const dInterval interval) const
    {
        return hit_functions[reference.pool_index](pool_pointers[reference.pool_index],
                                                   reference.object_index, ray, interval);
    }
};

using World = HittableList<Sphere>;
