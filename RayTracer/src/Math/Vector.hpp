#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <immintrin.h>
#include <utility>

#include "Numbers.hpp"
#include "Random.hpp"

/** Linear Algebra Library made to experiment with template metaprogramming */
// NOTE: As of C++26, <linalg> does a lot of this for you (See https://en.cppreference.com/cpp/numeric/linalg)

// <immintrin.h> requires x86 or x64 architecture
// clang-format off
#if defined(_WIN32)
    // Windows 32-bit (x86)
    #if defined(_M_IX86)
        static_assert(sizeof(void*) == 4, "Expected x86 architecture");
    // Windows 64-bit (x64 / AMD64)
    #elif defined(_M_X64)
        static_assert(sizeof(void*) == 8, "Expected x64 architecture");
    #else
        static_assert(false, "Unsupported Windows architecture");
    #endif

#elif defined(__unix__) || defined(__APPLE__)
    // Linux / macOS 32-bit (x86)
    #if defined(__i386__)
        static_assert(sizeof(void*) == 4, "Expected x86 architecture");
    // Linux / macOS 64-bit (x64)
    #elif defined(__x86_64__)
        static_assert(sizeof(void*) == 8, "Expected x64 architecture");
    #else
        static_assert(false, "Unsupported Unix/Apple architecture");
    #endif
#else
    static_assert(false, "Unsupported operating system");
#endif
// clang-format on

namespace Math
{

// Numbers are either floating points or integral types

template <Number T, std::size_t N>
struct Vector;

template <Number T, std::size_t N>
struct Color;

/** Base operations for any vector type using Curiously Recurring Template Pattern (CRTP) */
template <typename Derived, Number T, std::size_t N>
struct VectorOperations
{
    /* vector += vector using the [] operator of the derived class */
    constexpr Derived &operator+=(const Derived &other)
    {
        auto &self = static_cast<Derived &>(*this);
        auto add_vector = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] += other[Is]), ...); };
        add_vector(std::make_index_sequence<N>{});
        return self;
    }

    /* vector -= vector */
    constexpr Derived &operator-=(const Derived &other)
    {
        auto &self = static_cast<Derived &>(*this);
        auto subtract_vector = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] -= other[Is]), ...); };
        subtract_vector(std::make_index_sequence<N>{});
        return self;
    }

    /* vector *= vector */
    constexpr Derived &operator*=(const Derived &other)
    {
        auto &self = static_cast<Derived &>(*this);
        auto multiply_vector = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] *= other[Is]), ...); };
        multiply_vector(std::make_index_sequence<N>{});
        return self;
    }

    /* vector += scalar */
    constexpr Derived &operator+=(T scalar)
    {
        auto &self = static_cast<Derived &>(*this);
        auto add_scalar = [&]<std::size_t... Is>(std::index_sequence<Is...>) { ((self[Is] += scalar), ...); };
        add_scalar(std::make_index_sequence<N>{});
        return self;
    }

    /* vector -= scalar */
    constexpr Derived &operator-=(T scalar)
    {
        auto &self = static_cast<Derived &>(*this);
        auto subtract_scalar = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] -= scalar), ...); };
        subtract_scalar(std::make_index_sequence<N>{});
        return self;
    }

    /* vector *= scalar */
    constexpr Derived &operator*=(T scalar)
    {
        auto &self = static_cast<Derived &>(*this);
        auto multiply_scalar = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] *= scalar), ...); };
        multiply_scalar(std::make_index_sequence<N>{});
        return self;
    }

    /* vector /= scalar */
    constexpr Derived &operator/=(T scalar)
    {
        auto &self = static_cast<Derived &>(*this);
        auto divide_scalar = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] *= 1 / scalar), ...); };
        divide_scalar(std::make_index_sequence<N>{});
        return self;
    }

    // Binary operators (Hidden friends)
    /* vector != vector, vector == vector */
    [[nodiscard]] friend constexpr bool operator==(const Derived &left_vector, const Derived &right_vector)
    {
        auto downcast_left = static_cast<const Derived &>(left_vector);
        auto downcast_right = static_cast<const Derived &>(right_vector);
        return downcast_left.IsEqual(downcast_right);
    }

    /* vector <=> vector */
    [[nodiscard]] friend constexpr bool operator<=>(const Derived &left_vector, const Derived &right_vector)
    {
        auto downcast_left = static_cast<const Derived &>(left_vector);
        auto downcast_right = static_cast<const Derived &>(right_vector);
        return downcast_left.ThreeWayCompare(downcast_right);
    }

    /* vector + vector */
    friend constexpr Derived operator+(const Derived &left_vector, const Derived &right_vector)
    {
        auto result = left_vector;
        result += right_vector;
        return result;
    }

    /* vector - vector */
    friend constexpr Derived operator-(const Derived &left_vector, const Derived &right_vector)
    {
        auto result = left_vector;
        result -= right_vector;
        return result;
    }

    /* vector * vector */
    friend constexpr Derived operator*(const Derived &left_vector, const Derived &right_vector)
    {
        auto result = left_vector;
        result *= right_vector;
        return result;
    }

    /* vector + scalar */
    friend constexpr Derived operator+(const Derived &left_vector, T scalar)
    {
        auto result = left_vector;
        result += scalar;
        return result;
    }

    /* scalar + vector */
    friend constexpr Derived operator+(T scalar, const Derived &right_vector)
    {
        auto result = right_vector;
        result += scalar;
        return result;
    }

    /* vector - scalar */
    friend constexpr Derived operator-(const Derived &left_vector, T scalar)
    {
        auto result = left_vector;
        result -= scalar;
        return result;
    }

    /* vector * scalar */
    friend constexpr Derived operator*(const Derived &left_vector, T scalar)
    {
        auto result = left_vector;
        result *= scalar;
        return result;
    }

    /* scalar * vector */
    friend constexpr Derived operator*(T scalar, const Derived &right_vector)
    {
        auto result = right_vector;
        result *= scalar;
        return result;
    }

    /* vector / scalar */
    friend constexpr Derived operator/(const Derived &left_vector, T scalar)
    {
        auto result = left_vector;
        result /= scalar;
        return result;
    }

    /* -vector */
    friend constexpr Derived operator-(const Derived &right_vector)
    {
        auto result{right_vector};
        return -1 * result;
    }

    /*** Actual Functions ***/
    constexpr T MagnitudeSquared() const
    {
        const auto &self = static_cast<const Derived &>(*this);
        T sum{};
        auto accumulate = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((sum += self[Is] * self[Is]), ...); };
        accumulate(std::make_index_sequence<N>{});
        return sum;
    }

    constexpr T Magnitude() const { return std::sqrt(MagnitudeSquared()); }

    constexpr T InverseMagnitude() const { return static_cast<T>(1.0) / std::sqrt(MagnitudeSquared()); }

    constexpr Derived &Normalize()
    {
        auto &self = static_cast<Derived &>(*this);
        T magnitude_squared = self.MagnitudeSquared();
        if (magnitude_squared == static_cast<T>(0)) return self; // this prevents division by 0
        const T inverse_magnitude = self.InverseMagnitude();
        auto scale = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((self[Is] *= inverse_magnitude), ...); };
        scale(std::make_index_sequence<N>{});
        return self;
    }

    static constexpr Derived Normalize(const Derived &vector)
    {
        Derived result = vector;
        result.Normalize();
        return result;
    }

    static constexpr Derived UnitVector(const Derived &vector)
    {
        Derived result = vector;
        T magnitude_squared = result.MagnitudeSquared();
        if (magnitude_squared == static_cast<T>(0)) return result;
        return result / result.Magnitude();
    }

    constexpr T DotProduct(const Derived &vector) const
    {
        const auto &self = static_cast<const Derived &>(*this);
        T sum{};
        auto accumulate = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((sum += self[Is] * vector[Is]), ...); };
        accumulate(std::make_index_sequence<N>{});
        return sum;
    }

    static constexpr T DotProduct(const Derived &left_vector, const Derived &right_vector)
    {
        T sum{};
        auto accumulate = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((sum += left_vector[Is] * right_vector[Is]), ...); };
        accumulate(std::make_index_sequence<N>{});
        return sum;
    }

    static constexpr Derived GenerateRandomVector()
    {
        auto new_vector = Derived{};
        auto fill_vector = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((new_vector[Is] = Rand::GenerateRandomNumber<T>()), ...); };
        fill_vector(std::make_index_sequence<N>{});
        return new_vector;
    }

    static constexpr Derived GenerateRandomVector(T min, T max)
    {
        auto new_vector = Derived{};
        auto fill_vector = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { ((new_vector[Is] = Rand::GenerateRandomNumber<T>(min, max)), ...); };
        fill_vector(std::make_index_sequence<N>{});
        return new_vector;
    }

    // very simple rejection method, can be optimized
    static constexpr Derived GenerateRandomUnitVector()
    {
        while (true)
        {
            auto candidate = GenerateRandomVector(static_cast<T>(-1), static_cast<T>(1));
            T magnitude_squared = candidate.MagnitudeSquared();
            if (static_cast<T>(0) < magnitude_squared && magnitude_squared <= static_cast<T>(1))
            {
                return candidate / std::sqrt(magnitude_squared);
            }
        }
    }

    // compares all members of a floating point vector to a threshold, in this case 1e-8
    constexpr bool HasNearZeroFloatPointPrecision()
    {
        if (!std::is_floating_point_v<T>) return false;
        auto &self = static_cast<Derived &>(*this);
        auto compare_all_variables = [&]<std::size_t... Is>(std::index_sequence<Is...>)
        { return ((self[Is] < 1e-8) && ...); };
        compare_all_variables(std::make_index_sequence<N>());
        return false; // should never be reached
    }
};

// Primary template for arbitrary N
template <Number T, std::size_t N>
struct Vector : public VectorOperations<Vector<T, N>, T, N>
{
    std::array<T, N> data{};

    constexpr Vector() = default;

    constexpr Vector(T scalar) { data.fill(scalar); }

    [[nodiscard]] constexpr bool IsEqual(const Vector &other) const noexcept { return data == other.data; }

    // NOTE: this is not a complete implementation, especially for floating point types, but will likely not
    // be used / is not a priority
    [[nodiscard]] constexpr bool ThreeWayCompare(const Vector &other) const noexcept
    { return data <=> other.data; }

    template <typename Self>
    constexpr auto &&operator[](this Self &&self, std::size_t i)
    { return std::forward_like<Self>(self).data[i]; }
};

/*** Specialization for N = 2 ***/
// putting "using Vector2 = Vector<T, 2zu>" near "template<Number T>" makes the compiler fucking scream and
// try to kill you, DO NOT DO IT!!!!
template <Number T>
struct Vector<T, 2zu> : public VectorOperations<Vector<T, 2zu>, T, 2zu>
{
    T x{}, y{};

    constexpr Vector() = default;
    constexpr Vector(T _x, T _y) : x{_x}, y{_y} {}
    constexpr Vector(T scalar) : x{scalar}, y{scalar} {}

    [[nodiscard]] constexpr bool IsEqual(const Vector &other) const noexcept
    { return (x == other.x && y == other.y); }

    // NOTE: this is not a complete implementation, especially for floating point types, but will likely not
    // be used / is not a priority
    [[nodiscard]] constexpr bool ThreeWayCompare(const Vector &other) const noexcept
    { return (x <=> other.x && y <=> other.y); }

    template <typename Self>
    constexpr auto &&operator[](this Self &&self, std::size_t i)
    {
        if (i == 0) return std::forward_like<Self>(self).x;
        return std::forward_like<Self>(self).y;
    }
};

/*** Specialization for N = 3 ***/

template <typename Derived, Number T>
struct Math3D
{
    static constexpr Derived CrossProduct(const Derived &left, const Derived &right)
    {
        auto x_param = left.y * right.z - left.z * right.y;
        auto y_param = left.z * right.x - left.x * right.z;
        auto z_param = left.x * right.y - left.y * right.x;
        return {x_param, y_param, z_param};
    }

    // x: 0, y: 1, z: 2
    constexpr Derived CrossProduct(const Derived &other) const
    {
        const auto self = static_cast<const Derived &>(*this);
        auto x_param = self[1] * other[2] - self[2] * other[1];
        auto y_param = self[2] * other[0] - self[0] * other[2];
        auto z_param = self[0] * other[1] - self[1] * other[0];
        return Derived{x_param, y_param, z_param};
    }

    static constexpr Derived ReflectFromSurfaceNormal(const Derived &incoming_vector,
                                                      const Derived &surface_unit_vector)
    {
        assert(std::abs(surface_unit_vector.MagnitudeSquared() - static_cast<T>(1)) < 1e-5 &&
               "Vector::Reflect requires normalized surface_unit_vector argument.");
        return incoming_vector -
               2 * Derived::DotProduct(incoming_vector, surface_unit_vector) * surface_unit_vector;
    }

    static constexpr Derived RefractFromSurfaceNormal(const Derived &uv_vector,
                                                      const Derived &surface_unit_vector,
                                                      double etai_over_etat)
    {
        assert(std::abs(surface_unit_vector.MagnitudeSquared() - static_cast<T>(1)) < 1e-5 &&
               "Vector::Refract requires normalized surface_unit_vector argument.");
        auto cosine_theta = std::fmin(Derived::DotProduct(-uv_vector, surface_unit_vector), 1.0);
        auto refracted_out_perpendicular = etai_over_etat * (uv_vector + cosine_theta * surface_unit_vector);
        auto refracted_out_parallel =
            -std::sqrt(std::fabs(1.0f - refracted_out_perpendicular.MagnitudeSquared())) *
            surface_unit_vector;
        return refracted_out_perpendicular + refracted_out_parallel;
    }

    static constexpr Derived RandomUnitVectorOnHemisphere(const Derived &normal_vector)
    {
        // NOTE: this should change once std::sqrt() is constexpr
        assert(std::abs(normal_vector.MagnitudeSquared() - static_cast<T>(1)) < 1e-5 &&
               "Vector::RandomUnitVectorOnHemisphere requires normalized vector argument.");
        auto vector_on_unit_sphere = Derived::GenerateRandomUnitVector();
        // if we're on the same hemisphere as the normal
        if (Derived::DotProduct(vector_on_unit_sphere, normal_vector) > 0.0) return vector_on_unit_sphere;
        else return -vector_on_unit_sphere;
    }
};

// - See using Vector2 comment above
template <Number T>
struct Vector<T, 3zu> : public VectorOperations<Vector<T, 3zu>, T, 3zu>, public Math3D<Vector<T, 3zu>, T>
{
    T x{}, y{}, z{};

    constexpr Vector() = default;
    constexpr Vector(T _x, T _y, T _z) : x{_x}, y{_y}, z{_z} {}
    constexpr Vector(T scalar) : x{scalar}, y{scalar}, z{scalar} {}
    constexpr Vector(Color<T, 3zu> color) : x{color.r}, y{color.g}, z{color.b} {}

    [[nodiscard]] constexpr bool IsEqual(const Vector &other) const noexcept
    { return (x == other.x && y == other.y && z == other.z); }

    // NOTE: this is not a complete implementation, especially for floating point types, but will likely not
    // be used / is not a priority
    [[nodiscard]] constexpr bool ThreeWayCompare(const Vector &other) const noexcept
    { return (x <=> other.x && y <=> other.y && z <=> other.z); }

    template <typename Self>
    constexpr auto &&operator[](this Self &&self, std::size_t i)
    {
        if (i == 0) return std::forward_like<Self>(self).x;
        if (i == 1) return std::forward_like<Self>(self).y;
        return std::forward_like<Self>(self).z;
    }
};

template <Number T>
struct Color<T, 3zu> : public VectorOperations<Color<T, 3zu>, T, 3zu>, public Math3D<Color<T, 3zu>, T>
{
    T r{}, g{}, b{};

    constexpr Color() = default;
    constexpr Color(T _r, T _g, T _b) : r{_r}, g{_g}, b{_b} {}
    constexpr Color(T scalar) : r{scalar}, g{scalar}, b{scalar} {}
    constexpr Color(Vector<T, 3zu> vector) : r{vector.x}, g{vector.y}, b{vector.z} {}

    [[nodiscard]] constexpr bool IsEqual(const Color &other) const noexcept
    { return (r == other.r && g == other.g && b == other.b); }

    // NOTE: this is not a complete implementation, especially for floating point types, but will likely not
    // be used / is not a priority
    [[nodiscard]] constexpr bool ThreeWayCompare(const Color &other) const noexcept
    { return (r <=> other.r && g <=> other.g && b <=> other.b); }

    template <typename Self>
    constexpr auto &&operator[](this Self &&self, std::size_t i)
    {
        if (i == 0) return std::forward_like<Self>(self).r;
        if (i == 1) return std::forward_like<Self>(self).g;
        return std::forward_like<Self>(self).b;
    }
};

// Type aliases using the specialization directly
template <Number T>
using Vector2D = Vector<T, 2zu>;

template <Number T>
using Vector3D = Vector<T, 3zu>;

template <Number T>
using Color3D = Color<T, 3zu>;

template <Number T>
using Point2D = Vector<T, 2zu>;

template <Number T>
using Point3D = Vector<T, 3zu>;

} // namespace Math

template <Math::Number T, std::size_t N>
struct std::formatter<Math::Vector<T, N>>
{
    constexpr auto parse(std::format_parse_context &context) const { return std::begin(context); }
    constexpr auto format(const Math::Vector<T, N> &object, std::format_context &context) const
    {
        auto out = std::format_to(context.out(), "[");
        for (auto i{0zu}; i < N; ++i)
        {
            out = std::format_to(out, "{}{}", object[i], (i < N - 1) ? ", " : "");
        }
        return std::format_to(out, "]");
    }
};

/** Aliases */
template <Math::Number T>
using Vector3 = Math::Vector3D<T>;

template <Math::Number T>
using Vector2 = Math::Vector2D<T>;

template <Math::Number T>
using Point3 = Math::Vector3D<T>;

template <Math::Number T>
using Point2 = Math::Vector2D<T>;

template <Math::Number T>
using Color = Math::Vector3D<T>;

using iVector2 = Math::Vector2D<std::int32_t>;
using iVector3 = Math::Vector3D<std::int32_t>;

using uVector2 = Math::Vector2D<std::uint32_t>;
using uVector3 = Math::Vector3D<std::uint32_t>;

using dVector2 = Math::Vector2D<double>;
using dVector3 = Math::Vector3D<double>;

using fVector2 = Math::Vector2D<float>;
using fVector3 = Math::Vector3D<float>;

using iPoint2 = Math::Point2D<std::int32_t>;
using iPoint3 = Math::Point3D<std::int32_t>;

using uPoint2 = Math::Point2D<std::uint32_t>;
using uPoint3 = Math::Point3D<std::uint32_t>;

using dPoint2 = Math::Point2D<double>;
using dPoint3 = Math::Point3D<double>;

using fPoint2 = Math::Point2D<float>;
using fPoint3 = Math::Point3D<float>;

using fColor = Math::Color3D<float>;
using dColor = Math::Color3D<double>;
