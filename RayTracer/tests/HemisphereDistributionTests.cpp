#include <gtest/gtest.h>

#include <cmath>
#include <numeric>
#include <vector>

#include <glm/glm.hpp>
#include <Walnut/Random.h>

#include "Math/Vector.hpp"

// -- GenerateRandomVector(-1,1) Coverage and Bounds ----

TEST(RandomVector, GenerateRandomVectorMinusOneToOneCoversNegativeRange)
{
    // GenerateRandomVector(-1, 1) should produce components in [-1, 1].
    constexpr int iterations = 10'000;

    float observed_min_x = 1.0f;
    float observed_min_y = 1.0f;
    float observed_min_z = 1.0f;

    for (int i = 0; i < iterations; ++i)
    {
        auto v = fVector3::GenerateRandomVector(-1.0f, 1.0f);
        observed_min_x = std::min(observed_min_x, v.x);
        observed_min_y = std::min(observed_min_y, v.y);
        observed_min_z = std::min(observed_min_z, v.z);
    }

    // With 10k samples over [-1, 1], we should observe values below -0.5
    EXPECT_LT(observed_min_x, -0.5f) << "x component never goes below -0.5";
    EXPECT_LT(observed_min_y, -0.5f) << "y component never goes below -0.5";
    EXPECT_LT(observed_min_z, -0.5f) << "z component never goes below -0.5";
}

TEST(RandomVector, GenerateRandomVectorMinusOneToOneRespectsRange)
{
    // Every component must be within [-1, 1]
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        auto v = fVector3::GenerateRandomVector(-1.0f, 1.0f);
        ASSERT_GE(v.x, -1.0f) << "x out of range at iteration " << i;
        ASSERT_LE(v.x,  1.0f) << "x out of range at iteration " << i;
        ASSERT_GE(v.y, -1.0f) << "y out of range at iteration " << i;
        ASSERT_LE(v.y,  1.0f) << "y out of range at iteration " << i;
        ASSERT_GE(v.z, -1.0f) << "z out of range at iteration " << i;
        ASSERT_LE(v.z,  1.0f) << "z out of range at iteration " << i;
    }
}

// -- GenerateRandomUnitVector Octant Coverage --

TEST(RandomVector, GenerateRandomUnitVectorCoversAllOctants)
{
    // A uniform distribution on the unit sphere should cover all 8 octants.
    constexpr int iterations = 10'000;

    // Track which octants have been hit (3 sign bits → 8 octants)
    bool octant_hit[8] = {};

    for (int i = 0; i < iterations; ++i)
    {
        auto v = fVector3::GenerateRandomUnitVector();
        int octant = (v.x < 0 ? 1 : 0) | (v.y < 0 ? 2 : 0) | (v.z < 0 ? 4 : 0);
        octant_hit[octant] = true;
    }

    for (int o = 0; o < 8; ++o)
    {
        EXPECT_TRUE(octant_hit[o])
            << "Octant " << o << " (signs: "
            << ((o & 1) ? '-' : '+') << ((o & 2) ? '-' : '+') << ((o & 4) ? '-' : '+')
            << ") was never hit in " << iterations << " samples";
    }
}

TEST(RandomVector, GenerateRandomUnitVectorIsNormalized)
{
    // Every returned vector must have unit length
    constexpr int iterations = 10'000;

    for (int i = 0; i < iterations; ++i)
    {
        auto v = fVector3::GenerateRandomUnitVector();
        float mag_sq = v.MagnitudeSquared();
        ASSERT_NEAR(mag_sq, 1.0f, 1e-4f) << "Not unit length at iteration " << i;
    }
}

// -- Hemisphere Distribution -

TEST(RandomVector, HemisphereVectorIsOnCorrectSide)
{
    // All returned vectors should be on the same hemisphere as the normal
    constexpr int iterations = 10'000;
    fVector3 normal{ 0.0f, 1.0f, 0.0f };

    for (int i = 0; i < iterations; ++i)
    {
        auto v = fVector3::RandomUnitVectorOnHemisphere(normal);
        float dot = fVector3::DotProduct(v, normal);
        ASSERT_GE(dot, 0.0f) << "Vector on wrong hemisphere at iteration " << i;
    }
}

TEST(RandomVector, HemisphereVectorIsNormalized)
{
    constexpr int iterations = 10'000;
    fVector3 normal{ 0.0f, 1.0f, 0.0f };

    for (int i = 0; i < iterations; ++i)
    {
        auto v = fVector3::RandomUnitVectorOnHemisphere(normal);
        float mag_sq = v.MagnitudeSquared();
        ASSERT_NEAR(mag_sq, 1.0f, 1e-4f) << "Not unit length at iteration " << i;
    }
}

// -- Comparison: cosine-weighted distribution (normal + InUnitSphere) -------
//
// The Walnut approach: normalize(normal + InUnitSphere()) produces a
// cosine-weighted distribution where the average cos(θ) ≈ π/4 ≈ 0.785.
//
// A uniform hemisphere distribution has average cos(θ) = 0.5.
//
// We measure the average dot product (= average cosine) of each method
// against the normal and verify they are producing the expected distribution.

TEST(RandomVector, WalnutInUnitSphereProducesCosineWeightedDistribution)
{
    // Walnut's approach: normalize(normal + random_unit_sphere_vector)
    // Expected average cos(θ) ≈ 2/π ≈ 0.6366 for Lambertian
    // (Note: the exact value is 2/3 ≈ 0.6667 for the average dot product
    //  of the cosine-weighted hemisphere distribution)
    constexpr int iterations = 100'000;
    glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

    double sum_cos_theta = 0.0;

    for (int i = 0; i < iterations; ++i)
    {
        glm::vec3 direction = glm::normalize(normal + Walnut::Random::InUnitSphere());
        float cos_theta = glm::dot(direction, normal);
        sum_cos_theta += cos_theta;
    }

    double avg_cos = sum_cos_theta / iterations;
    // Cosine-weighted (Lambertian) distribution: average cos(θ) = 2/3 ≈ 0.6667
    EXPECT_NEAR(avg_cos, 2.0 / 3.0, 0.02)
        << "Walnut method should produce cosine-weighted distribution with avg cos ≈ 0.667";
}

TEST(RandomVector, CustomHemisphereProducesUniformDistribution)
{
    // RandomUnitVectorOnHemisphere produces
    // uniform hemisphere sampling with average cos(θ) = 0.5.
    constexpr int iterations = 100'000;
    fVector3 normal{ 0.0f, 1.0f, 0.0f };

    double sum_cos_theta = 0.0;

    for (int i = 0; i < iterations; ++i)
    {
        auto direction = fVector3::RandomUnitVectorOnHemisphere(normal);
        float cos_theta = fVector3::DotProduct(direction, normal);
        sum_cos_theta += cos_theta;
    }

    double avg_cos = sum_cos_theta / iterations;

    // Expected average cos(θ) for uniform hemisphere is 0.5.
    EXPECT_NEAR(avg_cos, 0.5, 0.02)
        << "Custom hemisphere should produce uniform distribution with avg cos ≈ 0.5. "
           "Actual avg cos(θ) = " << avg_cos;
}

// -- Direct comparison of the two approaches --------------------------------

TEST(RandomVector, WalnutVsCustomAverageCosineComparison)
{
    // Side-by-side comparison showing the two methods produce different
    // distributions.  Documents that the Walnut approach is cosine-weighted
    // (correct for Lambertian) while the custom approach is uniform.
    constexpr int iterations = 100'000;
    fVector3 normal_f{ 0.0f, 1.0f, 0.0f };
    glm::vec3 normal_g{ 0.0f, 1.0f, 0.0f };

    double sum_cos_walnut = 0.0;
    double sum_cos_custom = 0.0;

    for (int i = 0; i < iterations; ++i)
    {
        // Walnut: cosine-weighted Lambertian
        glm::vec3 dir_w = glm::normalize(normal_g + Walnut::Random::InUnitSphere());
        sum_cos_walnut += glm::dot(dir_w, normal_g);

        // Custom: uniform hemisphere
        auto dir_c = fVector3::RandomUnitVectorOnHemisphere(normal_f);
        sum_cos_custom += fVector3::DotProduct(dir_c, normal_f);
    }

    double avg_walnut = sum_cos_walnut / iterations;
    double avg_custom = sum_cos_custom / iterations;

    // The two averages should differ:
    // - Walnut ≈ 0.667 (cosine-weighted)
    // - Custom should be ≈ 0.5 (uniform hemisphere)
    EXPECT_NEAR(avg_walnut, 2.0 / 3.0, 0.02)
        << "Walnut avg cos should be ≈ 0.667 (cosine-weighted)";

    // For information: print both averages
    std::cout << "  Walnut avg cos(θ): " << avg_walnut << " (expected ~0.667 for Lambertian)\n";
    std::cout << "  Custom avg cos(θ): " << avg_custom << " (expected ~0.500 for uniform hemisphere)\n";
}
