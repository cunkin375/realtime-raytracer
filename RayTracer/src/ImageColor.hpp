#pragma once

#include "Math/Interval.hpp"
#include "Math/Vector.hpp"

#include "Util/Aliases.hpp"

#include <iostream>

namespace ImageColor
{

constexpr f64 LinearToGamma(f64 linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);
    return 0.0;
}

constexpr void WriteColor(std::ostream &out, const dColor &pixel_color)
{
    auto r = LinearToGamma(pixel_color.r);
    auto g = LinearToGamma(pixel_color.g);
    auto b = LinearToGamma(pixel_color.b);

    static const auto intensity = dInterval{ 0.000, 0.999 };
    i32 rbyte = i32(256 * intensity.Clamp(r));
    i32 gbyte = i32(256 * intensity.Clamp(g));
    i32 bbyte = i32(256 * intensity.Clamp(b));

    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

constexpr u32 ConvertToRGBA(fVector4 color)
{
    u8 r = static_cast<u8>(color[0] * 255.0f);
    u8 g = static_cast<u8>(color[1] * 255.0f);
    u8 b = static_cast<u8>(color[2] * 255.0f);
    u8 a = static_cast<u8>(color[3] * 255.0f);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

} // namespace ImageColor
