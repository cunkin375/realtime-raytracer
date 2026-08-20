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
    inline static std::uniform_int_distribution<std::mt19937::result_type> int_distribution_;
    inline static std::uniform_real_distribution<float> float_distribution_{ 0.0f, 1.0f };
    inline static std::uniform_real_distribution<double> double_distribution_{ 0.0, 1.0 };

public:
    /* Generates a random number between given min and max */
    template <Number T>
    static T GenerateRandomNumber(T min, T max)
    {
        if constexpr (std::integral<T>)
        {
            std::uniform_int_distribution<T> distribution{ min, max };
            return distribution(generator_);
        }
        else if constexpr (std::same_as<T, float>)
        {
            // Map [0, 1] to [min, max]
            return min + float_distribution_(generator_) * (max - min);
        }
        else
        {
            return static_cast<T>(min + double_distribution_(generator_) * (max - min));
        }
    }

    /* Generates a random number */
    template <Number T>
    static T GenerateRandomNumber()
    {
        if constexpr (std::integral<T>)
        {
            return int_distribution_(generator_);
        }
        else if constexpr (std::same_as<T, float>)
        {
            return float_distribution_(generator_);
        }
        else
        {
            return static_cast<T>(double_distribution_(generator_));
        }
    }

    /* Generates a random number between 0 and 1 */
    template <Number T>
    static T GenerateRandomNormalizedNumber()
    {
        if constexpr (std::integral<T>)
        {
            return static_cast<T>(0); // 0 or 1 is not meaningful for integers; return 0
        }
        else if constexpr (std::same_as<T, float>)
        {
            return float_distribution_(generator_);
        }
        else
        {
            return static_cast<T>(double_distribution_(generator_));
        }
    }
};

using Rand = Random;

} // namespace Math
