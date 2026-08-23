#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "Util/Path.hpp"

TEST(PathResolver, ResolvesExistingShaderPath)
{
    const auto resolved = Util::ResolvePath("RayTracer/assets/shaders/raytracer.hlsl");
    EXPECT_TRUE(std::filesystem::exists(resolved));
    EXPECT_TRUE(std::filesystem::is_regular_file(resolved));

    auto file = std::ifstream{ resolved, std::ios::binary };
    EXPECT_TRUE(file.is_open());
}

TEST(PathResolver, ResolvesShaderDirectory)
{
    const auto resolved = Util::ResolvePath("RayTracer/assets/shaders/");
    EXPECT_TRUE(std::filesystem::exists(resolved));
    EXPECT_TRUE(std::filesystem::is_directory(resolved));
}

TEST(PathResolver, FallsBackGracefullyOnNonexistentPath)
{
    const auto nonexistent = std::filesystem::path("does/not/exist/shader.hlsl");
    const auto resolved = Util::ResolvePath(nonexistent);
    EXPECT_EQ(resolved, nonexistent);
}
