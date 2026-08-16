#include <gtest/gtest.h>

#include <cassert>

#include "Math/Vector.hpp"

TEST(Vector3, NegateWithOtherVector) {
    const auto v1 = -fVector3{1.0f, 2.0f, 3.0f};
    const auto expected = fVector3{-1.0f, -2.0f, -3.0f};
    ASSERT_EQ(v1, expected);
}

TEST(Vector3, NegateWithSelf) {
    const auto v1 = -fVector3{1.0f, 2.0f, 3.0f};
    ASSERT_TRUE(v1.x == -1.0f && v1.y == -2.0f && v1.z == -3.0f);
}
