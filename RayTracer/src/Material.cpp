#include "Material.hpp"

#include "Hittable.hpp"
#include "Math/Random.hpp"
#include "Ray.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

namespace
{
using Attenuation = dColor;
using ScatteredRay = dRay;

// Shlick's approximation
static double Reflectance(f64 cosine, f64 refraction_index)
{
    auto r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0 * r0;
    return r0 + (1 - r0) * std::pow((1 - cosine), 5);
}

} // namespace

/*** Lambertian Material functions ***/
std::optional<ScatterRecord> Lambertian::Scatter(const dRay &ray_in, const HitRecord &record) const
{
    // Correct Lambertian Reflection
    auto scatter_direction = record.normal + dVector3::GenerateRandomUnitVector();
    // Uniform Lambertian Reflection
    // dVector3 scatter_direction = dVector3::RandomUnitVectorOnHemisphere(surface_hit_normal);
    if (scatter_direction.HasNearZeroFloatPointPrecision()) { scatter_direction = record.normal; }
    return ScatterRecord{
        Attenuation{texture_.Value(record.u_texture_coord, record.v_texture_coord, record.end_point)},
        ScatteredRay{record.end_point, scatter_direction, ray_in.time}};
}

/*** Metal Material functions ***/
std::optional<ScatterRecord> Metal::Scatter(const dRay &ray_in, const HitRecord &record) const
{
    auto reflected_ray = dVector3::ReflectFromSurfaceNormal(ray_in.direction, record.normal);
    reflected_ray = dVector3::UnitVector(reflected_ray) + (fuzz_ * dVector3::GenerateRandomUnitVector());
    return ScatterRecord{Attenuation{albedo_}, ScatteredRay{record.end_point, reflected_ray, ray_in.time}};
}

/*** Dialectric Material functions ***/
std::optional<ScatterRecord> Dielectric::Scatter(const dRay &ray_in, const HitRecord &record) const
{
    f64 final_refraction_index = record.front_face ? (1.0 / refraction_index_) : refraction_index_;
    auto unit_direction = dVector3::UnitVector(ray_in.direction);

    f64 cosine_theta = std::fmin(dVector3::DotProduct(-unit_direction, record.normal), 1.0);
    f64 sin_theta = std::sqrt(1.0 - cosine_theta * cosine_theta);

    auto direction = dVector3{};
    bool cannot_refract = final_refraction_index * sin_theta > 1.0;

    if (cannot_refract ||
        Reflectance(cosine_theta, final_refraction_index) > Math::Rand::GenerateRandomNormalizedNumber<f64>())
    {
        direction = dVector3::ReflectFromSurfaceNormal(unit_direction, record.normal);
    }
    else
    {
        direction = dVector3::RefractFromSurfaceNormal(unit_direction, record.normal, final_refraction_index);
    }
    return ScatterRecord{Attenuation{1.0, 1.0, 1.0}, ScatteredRay{record.end_point, direction, ray_in.time}};
}
