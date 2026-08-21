#pragma once

#include <concepts>
#include <random>

#include "Numbers.hpp"

namespace Math
{

class Random
{
    inline static std::uniform_int_distribution<std::mt19937::result_type> int_distribution_;
    inline static std::uniform_real_distribution<float> float_distribution_{ 0.0f, 1.0f };
    inline static std::uniform_real_distribution<double> double_distribution_{ 0.0, 1.0 };

    inline static std::normal_distribution<float> float_normal_distribution_{ 0.0f, 1.0f };
    inline static std::normal_distribution<double> double_normal_distribution_{ 0.0, 1.0 };

public:
    /* Generates a random number between given min and max */
    template <Number T>
    static T GenerateRandomNumber(T min, T max)
    {
        if constexpr (std::integral<T>)
        {
            std::uniform_int_distribution<T> distribution{ min, max };
            return distribution(Generator());
        }
        else if constexpr (std::same_as<T, float>)
        {
            // Map [0, 1] to [min, max]
            return min + float_distribution_(Generator()) * (max - min);
        }
        else
        {
            return static_cast<T>(min + double_distribution_(Generator()) * (max - min));
        }
    }

    /* Generates a random number */
    template <Number T>
    static T GenerateRandomNumber()
    {
        if constexpr (std::integral<T>)
        {
            return int_distribution_(Generator());
        }
        else if constexpr (std::same_as<T, float>)
        {
            return float_distribution_(Generator());
        }
        else
        {
            return static_cast<T>(double_distribution_(Generator()));
        }
    }

    /* Generates a random number between 0 and 1 */
    template <std::floating_point T>
    static T GenerateNumberInUnitInterval()
    {
        if constexpr (std::same_as<T, float>)
        {
            return float_distribution_(Generator());
        }
        else
        {
            return static_cast<T>(double_distribution_(Generator()));
        }
    }

    // seed is taken by reference to support multi-threaded operations
    template <std::floating_point T>
    static T FastUnitInterval(std::uint32_t &seed)
    {
        seed = PCG_Hash(seed);
        return static_cast<T>(seed) / static_cast<T>(std::numeric_limits<std::uint32_t>::max());
    }

    /* Generates a random number between 0 and 1 */
    template <std::floating_point T>
    static T GenerateNumberInUnitInterval_Gauss()
    {
        if constexpr (std::same_as<T, float>)
        {
            return float_normal_distribution_(Generator());
        }
        else
        {
            return static_cast<T>(double_normal_distribution_(Generator()));
        }
    }

    // source: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
    inline static std::uint32_t PCG_Hash(std::uint32_t input)
    {
        std::uint32_t state = input * 747796405u + 289133643u;
        std::uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803747u;
        return (word >> 22u) ^ word;
    }

private:
    // generator is not its own member variable here because this approach allows code to be compiled by GCC
    // and MSVC while keeping the generator thread_local
    static std::mt19937 &Generator()
    {
        thread_local std::mt19937 generator{ std::random_device{}() };
        return generator;
    }
};

using Rand = Random;

} // namespace Math
