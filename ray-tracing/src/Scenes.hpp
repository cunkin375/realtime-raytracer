#pragma once

#include <utility>

#include "Camera.hpp"
#include "Material.hpp"
#include "Math/Random.hpp"

#include "BVH.hpp"
#include "Hittable.hpp"
#include "Sphere.hpp"
#include "Texture.hpp"

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

namespace Scenes
{
using Radius = f64;
using Reflection = f64;
using RefractionIndex = f64;
using Scale = f64;
using AspectRatio = f64;
using VerticalFOV = f64;
using ImageWidth = std::size_t;
using SamplesPerPixel = std::size_t;

template <typename WorldType>
struct RenderContext
{
    WorldType world;
    Camera camera;
};

inline World InitiateThreeSphereScene()
{
    auto world = World{};

    auto center_sphere = Sphere{dPoint3{0, 0, -1.2}, Radius{0.5},
                                Material{std::in_place_type<Lambertian>, dColor{0.8, 0.0, 0.2}}};

    auto big_bottom_sphere = Sphere{dPoint3{0, -100.5, -1}, Radius{100},
                                    Material{std::in_place_type<Lambertian>, dColor{0.8, 0.8, 0.0}}};

    auto left_sphere = Sphere{dPoint3{-1, 0, -1}, Radius{0.5},
                              Material{std::in_place_type<Dielectric>, RefractionIndex{1.50}}};

    auto left_sphere_bubble = Sphere{dPoint3{-1, 0, -1}, Radius{0.4},
                                     Material{std::in_place_type<Dielectric>, RefractionIndex{1.00 / 1.50}}};

    auto right_sphere = Sphere{dPoint3{1, 0, -1}, Radius{0.5},
                               Material{std::in_place_type<Metal>, dColor{0.2, 0.2, 0.8}, Reflection{1.0}}};

    world.Add(center_sphere);
    world.Add(big_bottom_sphere);
    world.Add(left_sphere);
    world.Add(left_sphere_bubble);
    world.Add(right_sphere);

    return world;
}

inline World InitiateTwoSphereScene()
{
    auto world = World{};

    auto R = std::cos(std::numbers::pi / 4);

    auto sphere_left =
        Sphere{dPoint3{-R, 0, -1}, Radius{R}, Material{std::in_place_type<Lambertian>, dColor{0, 0, 1}}};

    auto sphere_right =
        Sphere{dPoint3{R, 0, -1}, Radius{R}, Material{std::in_place_type<Lambertian>, dColor{1, 0, 0}}};

    world.Add(sphere_left);
    world.Add(sphere_right);

    return world;
}

inline RenderContext<BVH<Sphere>> InitiateManySphereScene()
{
    // Init World
    auto world = World{};

    using namespace Textures;
    auto ground_object = Sphere{
        dPoint3{0, -1000, 0}, Radius{1000},
        Material{std::in_place_type<Lambertian>,
                 Textures::Checker<SolidColor>{Scale{1.0}, dColor{0.2, 0.3, 0.1}, dColor{0.9, 0.9, 0.9}}}};

    world.Add(ground_object);

    for (auto a{-11}; a < 11; ++a)
    {
        for (auto b{-11}; b < 11; ++b)
        {
            using namespace Math;
            auto choose_material = Rand::GenerateRandomNumber<f64>();
            auto center = dPoint3{a + 0.9 * Rand::GenerateRandomNumber<f64>(), 0.2,
                                  b + 0.8 * Rand::GenerateRandomNumber<f64>()};

            if ((center - dPoint3{4, 0.2, 0}).Magnitude() > 0.9)
            {
                auto sphere_material = Material{};

                if (choose_material < 0.8)
                {
                    /* diffuse */
                    auto albedo = dColor::GenerateRandomVector() * dColor::GenerateRandomVector();
                    sphere_material = Material{std::in_place_type<Lambertian>, albedo};
                    world.Add(Sphere{center, Radius{0.2}, sphere_material});

                    // These add motion blur for a ball moving from center to center2
                    // auto center2 = center + dVector3{0, Rand::GenerateRandomNumber<f64>(0, 0.5), 0};
                    // world.Add(Sphere{center, center2, Radius{0.2}, sphere_material});
                }
                else if (choose_material < 0.95)
                {
                    /* metal */
                    auto albedo = dColor::GenerateRandomVector(0.5, 1);
                    auto fuzz = Rand::GenerateRandomNumber<f64>(0, 0.5);
                    sphere_material = Material{std::in_place_type<Metal>, albedo, fuzz};
                    world.Add(Sphere{center, Radius{0.2}, sphere_material});
                }
                else
                {
                    /* glass */
                    sphere_material = Material{std::in_place_type<Dielectric>, RefractionIndex{1.5}};
                    world.Add(Sphere{center, Radius{0.2}, sphere_material});
                }
            }
        }
    }

    auto material1 = Material{std::in_place_type<Dielectric>, RefractionIndex{1.5}};
    world.Add(Sphere{dPoint3{0, 1, 0}, Radius{1.0}, material1});

    auto material2 = Material{std::in_place_type<Lambertian>, dColor{0.4, 0.2, 0.1}};
    world.Add(Sphere{dPoint3{-4, 1, 0}, Radius{1.0}, material2});

    auto material3 = Material{std::in_place_type<Metal>, dColor{0.7, 0.6, 0.5}, Reflection{0.0}};
    world.Add(Sphere{dPoint3{4, 1, 0}, Radius{1.0}, material3});

    // Init Camera
    auto camera = Camera{AspectRatio{16.0 / 9.0}, ImageWidth{1080}, SamplesPerPixel{25}, VerticalFOV{20}};

    camera.look_from = dPoint3{13, 2, 3};
    camera.look_at = dPoint3{0, 0, 0};
    camera.vertical_up = dVector3{0, 1, 0};

    camera.defocus_angle = 0.6;
    camera.focus_distance = 10.0;

    return {BVH{world}, camera};
}

inline RenderContext<BVH<Sphere>> InitiateTwoCheckeredSpheres()
{
    auto world = World{};

    using namespace Textures;
    auto checker_pattern =
        Textures::Checker<SolidColor>{Scale{0.32}, dColor{0.2, 0.3, 0.1}, dColor{0.9, 0.9, 0.9}};

    world.Add(Sphere{dPoint3{0, -10, 0}, 10, Material{std::in_place_type<Lambertian>, checker_pattern}});
    world.Add(Sphere{dPoint3{0, 10, 0}, 10, Material{std::in_place_type<Lambertian>, checker_pattern}});

    auto camera = Camera{AspectRatio{16.0 / 9.0}, ImageWidth{1080}, SamplesPerPixel{25}, VerticalFOV{20}};

    camera.look_from = dPoint3{13, 2, 3};
    camera.look_at = dPoint3{0, 0, 0};
    camera.vertical_up = dVector3{0, 1, 0};

    camera.defocus_angle = 0;

    return {world, camera};
}

} // namespace Scenes
