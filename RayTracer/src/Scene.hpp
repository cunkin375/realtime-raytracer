#pragma once

#include <vector>

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

struct Material
{
    fVector3 albedo{ 1.f };
    f32 roughness{ 1.f };
    f32 metallic{ 0.f };
};

struct Sphere
{
    fVector3 position{ 0.f, 0.f, 0.f };
    f32 radius = 0.5f;
    Material material;
};

struct Scene
{
    std::vector<Sphere> spheres;
};
