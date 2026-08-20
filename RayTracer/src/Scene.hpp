#pragma once

#include <vector>

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

struct Material
{
    fVector3 albedo{ 1.f };
    fVector3 emission_color{ 0.f };
    f32 roughness{ 1.f };
    f32 metallic{ 0.f };
    f32 emission_power{ 0.f };

    fVector3 GetEmmission() const { return emission_color * emission_power; }
};

struct Sphere
{
    fVector3 position{ 0.f, 0.f, 0.f };
    f32 radius = 0.5f;
    i32 material_index{ 0 }; // ImGui works with ints
};

struct Scene
{
    std::vector<Sphere> spheres;
    std::vector<Material> materials;
    fVector3 light_direction{ -1, -1, -1 };
};
