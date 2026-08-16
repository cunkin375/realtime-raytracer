#include "Sphere.hpp"

#include <cmath>

#include "AABB.hpp"
#include "Hittable.hpp"
#include "Material.hpp"

#include "Math/Interval.hpp"
#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

namespace
{
// u, v are in [0, 1]
// point is a point belonging to a given sphere
static void GetSphereUV(const dPoint3 &point, f64 &u, f64 &v)
{
    f64 pi = std::numbers::pi;
    auto theta = std::acos(-point.y);
    auto phi = std::atan2(-point.z, point.x + pi);
    u = phi / (2 * pi);
    v = theta / pi;
}
} // namespace

// stationary sphere
Sphere::Sphere(const dPoint3 &static_center, f64 radius, Material material)
    : center_{static_center, dVector3{0, 0, 0}}, radius_{std::fmax(0, radius)}, material_{material}
{
    auto radius_vector = dVector3{radius, radius, radius};
    bounding_box_ = AABB{static_center - radius_vector, static_center + radius_vector};
}

// moving sphere
Sphere::Sphere(const dPoint3 &center1, const dPoint3 &center2, f64 radius, Material material_)
    : center_{center1, center2 - center1}, radius_{std::fmax(0, radius)}, material_{material_}
{
    auto radius_vector = dVector3{radius, radius, radius};
    auto box_1 = AABB<f64>{center_.At(0) - radius_vector, center_.At(0) + radius_vector};
    auto box_2 = AABB<f64>{center_.At(1) - radius_vector, center_.At(1) + radius_vector};
    bounding_box_ = AABB<f64>{box_1, box_2};
}

/* Hit function that solves quadratic with DotProduct(direction, origin center)->double */
std::optional<HitRecord> Sphere::Hit(const dRay &ray, dInterval ray_interval) const
{
    dPoint3 current_center = center_.At(ray.time);
    dVector3 origin_center = current_center - ray.origin;
    f64 a = ray.direction.MagnitudeSquared();
    f64 h = dVector3::DotProduct(ray.direction, origin_center);
    f64 c = origin_center.MagnitudeSquared() - radius_ * radius_;
    f64 discriminant = h * h - a * c;
    if (discriminant < 0) return std::nullopt;

    f64 square_root_discriminant = std::sqrt(discriminant);

    // find nearest root
    f64 root = (h - square_root_discriminant) / a;
    if (!ray_interval.Surrounds(root))
    {
        root = (h + square_root_discriminant) / a;
        if (!ray_interval.Contains(root)) return std::nullopt;
    }

    auto temp_record = HitRecord{};

    temp_record.distance = root;
    temp_record.end_point = ray.At(temp_record.distance);

    dVector3 outward_normal = (temp_record.end_point - current_center) / radius_;
    temp_record.SetFrontfaceNormal(ray, outward_normal);
    GetSphereUV(outward_normal, temp_record.u_texture_coord, temp_record.v_texture_coord);
    temp_record.material_view = &material_;

    return temp_record;
}
