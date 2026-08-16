#pragma once

#include "Vector.hpp"

#include <array>
#include <format>
#include <span>

namespace Math
{

// NOTE: this matrix implementation is column-major
struct Matrix4D
{
private:
    std::array<float, 16zu> data_;

public:
    constexpr Matrix4D()
        : data_{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}
    {
    }

    constexpr Matrix4D(const std::array<float, 16zu> &array) : data_{array} {}

    constexpr Matrix4D(const std::array<float, 16zu> &&array) : data_{std::move(array)} {}

    /* matrix *= matrix */
    // Matrix4 should be 64-bytes, which fits into an entire cache line
    // - cache friendly setups don't really matter here
    constexpr Matrix4D &operator*=(const Matrix4D &right)
    {
        std::array<float, 16zu> result_data{};
        for (auto column{0zu}; column < 4; ++column)
        {
            for (auto k{0zu}; k < 4; ++k)
            {
                float right_value = right.data_[k + column * 4];
                for (auto row{0zu}; row < 4; ++row)
                {
                    result_data[row + column * 4] += data_[row + k * 4] * right_value;
                }
            }
        }
        data_ = std::move(result_data);
        return *this;
    }

    /* matrix * matrix */
    friend constexpr Matrix4D operator*(const Matrix4D &left_matrix, const Matrix4D &right_matrix)
    {
        Matrix4D product = left_matrix;
        product *= right_matrix;
        return product;
    }

    /* matrix != matrix, matrix == matrix */
    [[nodiscard]] friend constexpr bool operator==(const Matrix4D &left_matrix,
                                                   const Matrix4D &right_matrix) = default;

    /* matrix <=> matrix */
    [[nodiscard]] friend constexpr bool operator<=>(const Matrix4D &left_matrix,
                                                    const Matrix4D &right_matrix) = default;

    constexpr Matrix4D(fVector3 translation)
        : data_{1.0f, 0.0f, 0.0f, 0.0f, 0.0f,          1.0f,          0.0f,          0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, translation.x, translation.y, translation.z, 1.0f}
    {
    }

    // TODO: make this constexpr? Not a priority
    static constexpr Matrix4D LookAt(const fVector3 &eye, const fVector3 &look_at, const fVector3 &up)
    {
        const auto forward = fVector3::Normalize(eye - look_at);
        const auto right = fVector3::CrossProduct(up, forward).Normalize();
        const auto true_up = fVector3::CrossProduct(forward, right).Normalize();
        const auto matrix = Matrix4D{{right.x, true_up.x, forward.x, 0.0f, right.y, true_up.y, forward.y,
                                      0.0f, right.z, true_up.z, forward.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
        return matrix * Matrix4D{-eye};
    }

    // NOTE: jesus christ this is ugly, but it runs!
    static constexpr Matrix4D Perspective(float fov, float width, float height, float near_plane,
                                          float far_plane)
    {
        const auto aspect_ratio = width / height;
        const auto temp = std::tan(fov / 2.0f);
        const auto t = temp * near_plane;
        const auto b = -t;
        const auto r = t * aspect_ratio;
        const auto l = b * aspect_ratio;
        return Matrix4D{{(2.0f * near_plane) / (r - l), 0.0f, 0.0f, 0.0f, 0.0f, (2.0f * near_plane) / (t - b),
                         0.0f, 0.0f, (r + l) / (r - l), (t + b) / (t - b),
                         -(far_plane + near_plane) / (far_plane - near_plane), -1.0f, 0.0f, 0.0f,
                         -(2.0f * far_plane * near_plane) / (far_plane - near_plane), 0.0f}};
    }

    constexpr std::span<const float> GetSpan() const noexcept { return data_; }
};

} // namespace Math

using Matrix4 = Math::Matrix4D;
static_assert(sizeof(Matrix4) == 64, "Matrix4 not properly aligned.");

template <>
struct std::formatter<Matrix4>
{
    constexpr auto parse(std::format_parse_context &context) const { return std::begin(context); }
    constexpr auto format(const Matrix4 &object, std::format_context &context) const
    {
        const auto *data = object.GetSpan().data();
        /* 0  4  8  12  1  5  9  13  2  6  10 14  3  7  11 15 */
        return std::format_to(context.out(), "[\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n]",
                              data[0], data[4], data[8], data[12], data[1], data[5], data[9], data[13],
                              data[2], data[6], data[10], data[14], data[3], data[7], data[11], data[15]);
    }
};
