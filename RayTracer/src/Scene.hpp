#pragma once

#include <vector>

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

struct Sphere
{
    fVector3 position{ 0.f, 0.f, 0.f };
    f32 radius = 0.5f;
    fColor albedo{ 1, 0, 1 };
};

struct Scene
{
    std::vector<Sphere> spheres;
};
