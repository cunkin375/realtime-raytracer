#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "Math/Random.hpp"

using Rand = Math::Random;

// -- GenerateRandomNumber(min, max) ------------------------------------------

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

// -- GenerateNumberInUnitInterval ------------------------------------------

TEST(Random, NormalizedFloatReturnsWithinZeroOne)
{
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::GenerateNumberInUnitInterval<float>();
        ASSERT_GE(value, 0.0f) << "Iteration " << i;
        ASSERT_LE(value, 1.0f) << "Iteration " << i;
    }
}

TEST(Random, NormalizedDoubleReturnsWithinZeroOne)
{
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        double value = Rand::GenerateNumberInUnitInterval<double>();
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
        float value = Rand::GenerateNumberInUnitInterval<float>();
        observed_min = std::min(observed_min, value);
        observed_max = std::max(observed_max, value);
    }

    EXPECT_LT(observed_min, 0.05f) << "Distribution does not cover lower range";
    EXPECT_GT(observed_max, 0.95f) << "Distribution does not cover upper range";
}

// -- FastUnitInterval & PCG_Hash ---------------------------------------------

TEST(Random, FastUnitIntervalFloatRespectsRange)
{
    constexpr int iterations = 10'000;
    std::uint32_t seed = 123456789u;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::FastUnitInterval<float>(seed);
        ASSERT_GE(value, 0.0f) << "Iteration " << i;
        ASSERT_LE(value, 1.0f) << "Iteration " << i;
    }
}

TEST(Random, FastUnitIntervalDoubleRespectsRange)
{
    constexpr int iterations = 10'000;
    std::uint32_t seed = 987654321u;

    for (int i = 0; i < iterations; ++i)
    {
        double value = Rand::FastUnitInterval<double>(seed);
        ASSERT_GE(value, 0.0) << "Iteration " << i;
        ASSERT_LE(value, 1.0) << "Iteration " << i;
    }
}

TEST(Random, FastUnitIntervalFloatCoversRange)
{
    constexpr int iterations = 10'000;
    std::uint32_t seed = 42u;

    float observed_min = 1.0f;
    float observed_max = 0.0f;

    for (int i = 0; i < iterations; ++i)
    {
        float value = Rand::FastUnitInterval<float>(seed);
        observed_min = std::min(observed_min, value);
        observed_max = std::max(observed_max, value);
    }

    EXPECT_LT(observed_min, 0.05f) << "Distribution does not cover lower range";
    EXPECT_GT(observed_max, 0.95f) << "Distribution does not cover upper range";
}

TEST(Random, FastUnitIntervalFloatMeanIsUniform)
{
    constexpr int iterations = 100'000;
    std::uint32_t seed = 1337u;

    double sum = 0.0;
    for (int i = 0; i < iterations; ++i)
    {
        sum += Rand::FastUnitInterval<float>(seed);
    }

    double mean = sum / iterations;
    // Expected mean for uniform [0, 1] is 0.5
    EXPECT_NEAR(mean, 0.5, 0.01) << "FastUnitInterval distribution mean deviates from uniform";
}

TEST(Random, FastUnitIntervalMutatesSeed)
{
    std::uint32_t initial_seed = 12345u;
    std::uint32_t seed = initial_seed;

    Rand::FastUnitInterval<float>(seed);
    EXPECT_NE(seed, initial_seed) << "FastUnitInterval must mutate the seed passed by reference";
}

// -- GenerateRandomNumber() (no args) ---------------------------------------

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

// -- Thread-local generator isolation ----------------------------------------

TEST(Random, ThreadLocalGeneratorIndependentAcrossThreads)
{
    // Each thread generates values independently; if generator_ were a shared
    // (non-thread-local) static there would be a data race and this test would
    // crash or produce wrong results under sanitisers.
    constexpr int num_threads = 8;
    constexpr int iterations  = 10'000;

    std::vector<std::thread> threads;
    std::atomic<int> failures{ 0 };

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&failures]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                float value = Rand::GenerateNumberInUnitInterval<float>();
                if (value < 0.0f || value > 1.0f)
                    ++failures;
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(failures.load(), 0)
        << "One or more threads produced out-of-range values; "
           "thread-local generator may not be isolated correctly.";
}

TEST(Random, ThreadLocalDistributionProducesDistinctSequences)
{
    // Verifies that each thread gets its own distribution state, not a shared one.
    // When distributions were `inline static` (shared), all threads using the same
    // default-seeded thread_local generator would read/write the same distribution
    // cache, causing them to produce identical (or corrupted) sequences.
    constexpr int num_threads = 8;
    constexpr int samples_per_thread = 100;

    // Each thread collects its own sequence of random floats
    std::vector<std::vector<float>> per_thread_values(num_threads);
    for (auto &v : per_thread_values)
        v.resize(samples_per_thread);

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&per_thread_values, t]()
        {
            for (int i = 0; i < samples_per_thread; ++i)
            {
                per_thread_values[t][i] = Rand::GenerateRandomNumber<float>(-0.5f, 0.5f);
            }
        });
    }

    for (auto &thread : threads)
        thread.join();

    // Count how many thread pairs have completely identical sequences.
    // With correct thread_local distributions, sequences should differ.
    int identical_pairs = 0;
    for (int a = 0; a < num_threads; ++a)
    {
        for (int b = a + 1; b < num_threads; ++b)
        {
            if (per_thread_values[a] == per_thread_values[b])
                ++identical_pairs;
        }
    }

    // With independent RNGs, the probability of two 100-sample float sequences
    // being identical is effectively zero. Allow 0 identical pairs.
    EXPECT_EQ(identical_pairs, 0)
        << "Multiple threads produced identical random sequences; "
           "distribution state is likely shared instead of thread-local.";
}
