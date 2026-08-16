#include <concepts>
#include <cstddef>

// This is not used anymore but kept for reference
namespace Old
{

namespace [[deprecated("Use Vector.hpp instead!!!")]] Math
{

    // Numbers are either floating points or integral types
    template <typename T>
    concept Number = std::floating_point<T> || std::integral<T>;

    // Forward Declaration
    template <std::size_t N, Number T>
    struct Vector;

    // 3D Vector of some Number Type
    // NOTE: Number is used to declare a template that contains only Number Types
    template <Number T>
    struct Vector<3zu, T>
    {
        T x{}, y{}, z{};

        // NOTE: constexpr allows for compile-time evaluation of constructors and functions
        constexpr Vector() = default;

        // Scalar copy
        constexpr Vector(T scalar) : x(scalar), y(scalar), z(scalar) {}

        // Vector copy
        constexpr Vector(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

        // returns reference to modified vector type
        constexpr Vector &operator+=(const Vector &right_vector)
        {
            x += right_vector.x;
            y += right_vector.y;
            z += right_vector.z;
            return *this; // <- do not change, this prevents undefined behavior
        }

        constexpr Vector &operator*=(const T right_number)
        {
            x *= right_number;
            y *= right_number;
            z *= right_number;
            return *this;
        }

        // Hidden friend
        // - Only evaluated if used
        // - Ex of Argument-Dependeint Lookup (ADL)
        friend constexpr Vector operator+(Vector left_vector, const Vector &right_vector)
        { return left_vector += right_vector; }

        friend constexpr Vector operator*(Vector vector, T scalar) { return vector *= scalar; }
    };

    // 2D Vector of some Number Type
    template <Number T>
    struct Vector<2zu, T>
    {
        T x{}, y{};

        constexpr Vector() = default;
        constexpr Vector(T scalar) : x(scalar), y(scalar) {}
        constexpr Vector(T _x, T _y) : x(_x), y(_y) {}

        constexpr Vector &operator+=(const Vector &right)
        {
            x += right.x;
            y += right.y;
            return *this;
        }

        constexpr Vector &operator*=(const T right)
        {
            x *= right;
            y *= right;
            return *this;
        }

        friend constexpr Vector operator+(Vector left, const Vector &right) { return left += right; }

        friend constexpr Vector operator*(Vector vector, T scalar)
        { return Vector(vector.x * scalar, vector.y * scalar); }
    };

} // namespace Math

} // namespace Old
