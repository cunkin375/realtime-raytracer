#pragma once

#include <concepts>
#include <random>

#include "Numbers.hpp"

namespace Math
{

class Random
{
private:
    inline static std::mt19937 generator_;
    inline static std::uniform_int_distribution<std::mt19937::result_type> distribution_;

public:
    /* Generates a random number between given min and max */
    template <Number T>
    static constexpr T GenerateRandomNumber(T min, T max)
    {
        if constexpr (std::integral<T>) return distribution_(generator_);
        else return static_cast<T>(distribution_(generator_));
    }

    /* Generates a random number */
    template <Number T>
    static constexpr T GenerateRandomNumber()
    {
        if constexpr (std::integral<T>) return distribution_(generator_);
        else return static_cast<T>(distribution_(generator_));
    }

    /* Generates a random number between 0 and 1 */
    template <Number T>
    static constexpr T GenerateRandomNormalizedNumber()
    {
        if constexpr (std::integral<T>) return distribution_(generator_);
        else return static_cast<T>(distribution_(generator_));
    }
};

using Rand = Random;

} // namespace Math
