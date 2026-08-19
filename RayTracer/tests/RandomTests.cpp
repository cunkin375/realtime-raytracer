#include <gtest/gtest.h>

#include "Math/Random.hpp"

using Rand = Math::Random;

// ── GenerateRandomNumber(min, max) ──────────────────────────────────────────

TEST(Random, FloatMinMaxRespectsRange)
{
    constexpr float min = -0.5f;
    constexpr float max = 0.5f;
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::GenerateRandomNumber<float>(min, max);
        ASSERT_GE(value, min) << "Iteration " << i;
        ASSERT_LE(value, max) << "Iteration " << i;
    }
}

TEST(Random, DoubleMinMaxRespectsRange)
{
    constexpr double min = -100.0;
    constexpr double max = 100.0;
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        double value = Rand::GenerateRandomNumber<double>(min, max);
        ASSERT_GE(value, min) << "Iteration " << i;
        ASSERT_LE(value, max) << "Iteration " << i;
    }
}

TEST(Random, IntMinMaxRespectsRange)
{
    constexpr int min = -10;
    constexpr int max = 10;
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        int value = Rand::GenerateRandomNumber<int>(min, max);
        ASSERT_GE(value, min) << "Iteration " << i;
        ASSERT_LE(value, max) << "Iteration " << i;
    }
}

TEST(Random, FloatMinMaxCoversRange)
{
    constexpr float min = -0.5f;
    constexpr float max = 0.5f;
    constexpr int iterations = 10'000;

    float observed_min = max;
    float observed_max = min;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::GenerateRandomNumber<float>(min, max);
        observed_min = std::min(observed_min, value);
        observed_max = std::max(observed_max, value);
    }

    // With 10k samples over [-0.5, 0.5] we should get within 0.05 of each bound
    EXPECT_LT(observed_min, min + 0.05f) << "Distribution does not cover lower range";
    EXPECT_GT(observed_max, max - 0.05f) << "Distribution does not cover upper range";
}

// ── GenerateRandomNormalizedNumber ──────────────────────────────────────────

TEST(Random, NormalizedFloatReturnsWithinZeroOne)
{
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::GenerateRandomNormalizedNumber<float>();
        ASSERT_GE(value, 0.0f) << "Iteration " << i;
        ASSERT_LE(value, 1.0f) << "Iteration " << i;
    }
}

TEST(Random, NormalizedDoubleReturnsWithinZeroOne)
{
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        double value = Rand::GenerateRandomNormalizedNumber<double>();
        ASSERT_GE(value, 0.0) << "Iteration " << i;
        ASSERT_LE(value, 1.0) << "Iteration " << i;
    }
}

TEST(Random, NormalizedFloatCoversRange)
{
    constexpr int iterations = 10'000;

    float observed_min = 1.0f;
    float observed_max = 0.0f;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::GenerateRandomNormalizedNumber<float>();
        observed_min = std::min(observed_min, value);
        observed_max = std::max(observed_max, value);
    }

    EXPECT_LT(observed_min, 0.05f) << "Distribution does not cover lower range";
    EXPECT_GT(observed_max, 0.95f) << "Distribution does not cover upper range";
}

// ── GenerateRandomNumber() (no args) ───────────────────────────────────────

TEST(Random, ParameterlessFloatReturnsWithinZeroOne)
{
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::GenerateRandomNumber<float>();
        ASSERT_GE(value, 0.0f) << "Iteration " << i;
        ASSERT_LE(value, 1.0f) << "Iteration " << i;
    }
}
