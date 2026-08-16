#pragma once

#include <concepts>
#include <random>

#include "Numbers.hpp"

namespace Math::Rand
{

/* Generates a random number between given min and max */
template <Number T>
constexpr T GenerateRandomNumber(T min, T max)
{
    thread_local static auto random_device = std::random_device{};
    thread_local static auto generator = std::mt19937{random_device()};
    if constexpr (std::integral<T>)
    {
        auto distribution = std::uniform_int_distribution<T>{min, max};
        return distribution(generator);
    }
    else
    {
        auto distribution = std::uniform_real_distribution<T>{min, max};
        return distribution(generator);
    }
}

/* Generates a random number */
template <Number T>
constexpr T GenerateRandomNumber()
{
    thread_local static auto random_device = std::random_device{};
    thread_local static auto generator = std::mt19937{random_device()};
    if constexpr (std::integral<T>)
    {
        auto distribution = std::uniform_int_distribution<T>{};
        return distribution(generator);
    }
    else
    {
        auto distribution = std::uniform_real_distribution<T>{};
        return distribution(generator);
    }
}

/* Generates a random number between 0 and 1 */
template <Number T>
constexpr T GenerateRandomNormalizedNumber()
{
    thread_local static auto random_device = std::random_device{};
    thread_local static auto generator = std::mt19937{random_device()};
    if constexpr (std::integral<T>)
    {
        auto distribution = std::uniform_int_distribution<T>{0, 1};
        return distribution(generator);
    }
    else
    {
        auto distribution = std::uniform_real_distribution<T>{0, 1};
        return distribution(generator);
    }
}

} // namespace Math::Rand
