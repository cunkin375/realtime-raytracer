#pragma once

#include "AABB.hpp"
#include "Material.hpp"
#include "Ray.hpp"

#include "Util/Aliases.hpp"

class Sphere
{
private:
    dRay center_;
    f64 radius_;
    Material material_;
    AABB<f64> bounding_box_;

public:
    // stationary sphere
    Sphere(const dPoint3 &static_center, f64 radius, Material material_);

    // moving sphere
    Sphere(const dPoint3 &center1, const dPoint3 &center2, f64 radius, Material material_);

    AABB<f64> BoundingBox() const noexcept { return bounding_box_; };

    /* Hit function that solves quadratic with DotProduct(direction, origin center)->double */
    std::optional<HitRecord> Hit(const dRay &ray, dInterval ray_interval) const;
};
