#pragma once

#include <concepts>
#include <numbers>

namespace Math
{

template <typename T>
concept Number = std::floating_point<T> || std::integral<T>;

template <typename T>
    requires std::floating_point<T>
constexpr double DegreesToRadians(T degrees)
{ return degrees * (std::numbers::pi / 180); }

} // namespace Math
