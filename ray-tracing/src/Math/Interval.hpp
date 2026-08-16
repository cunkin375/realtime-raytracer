#pragma once

#include "Numbers.hpp"

#include <format>
#include <limits>
#include <stdint.h>

namespace Math
{

template <Number T>
struct Interval
{
public:
    T lower{}, upper{};

public:
    // this should represent empty constant
    constexpr Interval() : lower{PositiveInfinity()}, upper{NegativeInfinity()} {}

    constexpr Interval(T lower, T upper) : lower{lower}, upper{upper} {}

    constexpr Interval(const Interval &left, const Interval &right)
    {
        lower = left.lower <= right.lower ? left.lower : right.lower;
        upper = left.upper >= right.upper ? left.upper : right.upper;
    }

    static constexpr T NegativeInfinity() noexcept
    {
        if constexpr (std::numeric_limits<T>::has_infinity) return -std::numeric_limits<T>::infinity();
        else return std::numeric_limits<T>::lowest();
    }

    static constexpr T PositiveInfinity() noexcept
    {
        if constexpr (std::numeric_limits<T>::has_infinity) return std::numeric_limits<T>::infinity();
        else return std::numeric_limits<T>::max();
    }

    constexpr double Size() const noexcept { return upper - lower; }

    constexpr bool Contains(T x) const noexcept { return lower <= x && x <= upper; }

    constexpr bool Surrounds(T x) const noexcept { return lower < x && x < upper; }

    constexpr T Clamp(T number) const
    {
        if (number < lower) return lower;
        if (number > upper) return upper;
        return number;
    }

    static constexpr T Clamp(T number, T low, T high)
    {
        if (number < low) return low;
        if (number > high) return number;
        return high - 1;
    }

    constexpr Interval Expand(T delta) const
    {
        auto padding = delta / 2;
        return {lower - padding, upper + padding};
    }

    static constexpr Interval Empty()    { return {.lower = PositiveInfinity(), .upper = NegativeInfinity()}; }
    static constexpr Interval Universe() { return {.lower = NegativeInfinity(), .upper = PositiveInfinity()}; }
};

} // namespace Math

template <Math::Number T>
struct std::formatter<Math::Interval<T>>
{
    constexpr auto parse(std::format_parse_context &context) const { return std::begin(context); }
    constexpr auto format(const Math::Interval<T> &interval, std::format_context &context) const
    {
        auto out = std::format_to(context.out(), "(");
        out = std::format_to(out, "{}, {}", interval.lower, interval.upper);
        return std::format_to(out, ")");
    }
};

/* Aliases */
template <Math::Number T>
using Interval = Math::Interval<T>;

using fInterval = Math::Interval<float>;
using dInterval = Math::Interval<double>;
using uInterval = Math::Interval<std::uint32_t>;
using iInterval = Math::Interval<std::int32_t>;
