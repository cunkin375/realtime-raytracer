#pragma once

#include <optional>
#include <variant>

#include "Ray.hpp"
#include "Texture.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

struct HitRecord;

struct ScatterRecord
{
    dColor attenuation;
    dRay scattered_ray;
};

struct Lambertian
{
private:
    Texture texture_{};

public:
    Lambertian() = default;

    Lambertian(const dColor &albedo) : texture_{albedo} {}
    Lambertian(const Texture &texture) : texture_{texture} {}

    std::optional<ScatterRecord> Scatter(const dRay &ray_in, const HitRecord &record) const;
};

struct Metal
{
private:
    dColor albedo_{0.5};
    f64 fuzz_{1.0};

public:
    Metal() = default;

    Metal(const dColor &albedo) : albedo_{albedo} {}

    Metal(const dColor &albedo, f64 fuzz) : albedo_{albedo}, fuzz_{fuzz} {}

    std::optional<ScatterRecord> Scatter(const dRay &ray_in, const HitRecord &record) const;
};

struct Dielectric
{
private:
    // refractive index in vacuum or air, or the ratio of material's refractive index over refractive index of
    // the enclosing media
    f64 refraction_index_;

public:
    Dielectric() = default;

    Dielectric(f64 refraction_index) : refraction_index_{refraction_index} {}

    std::optional<ScatterRecord> Scatter(const dRay &ray_in, const HitRecord &record) const;
};

using Material = std::variant<Lambertian, Metal, Dielectric>;
