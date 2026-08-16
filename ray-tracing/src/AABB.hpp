#pragma once

#include "Util/Aliases.hpp"

#include "Math/Interval.hpp"
#include "Math/Numbers.hpp"
#include "Math/Vector.hpp"
#include "Ray.hpp"

template <Math::Number T>
struct AxisAlignedBoundingBox
{

public:
    Interval<T> x, y, z;

    constexpr AxisAlignedBoundingBox() = default;

    constexpr AxisAlignedBoundingBox(const Interval<T> &_x, const Interval<T> &_y, const Interval<T> &_z)
        : x{_x}, y{_y}, z{_z}
    {
    }

    constexpr AxisAlignedBoundingBox(const Point3<T> &point_a, const Point3<T> &point_b)
    {
        // treat both points as the extremes of the bounding box
        x = (point_a.x <= point_b.x) ? Interval<T>{point_a.x, point_b.x} : Interval<T>{point_b.x, point_a.x};
        y = (point_a.y <= point_b.y) ? Interval<T>{point_a.y, point_b.y} : Interval<T>{point_b.y, point_a.y};
        z = (point_a.z <= point_b.z) ? Interval<T>{point_a.z, point_b.z} : Interval<T>{point_b.z, point_a.z};
    }

    constexpr AxisAlignedBoundingBox(const AxisAlignedBoundingBox &box_0, const AxisAlignedBoundingBox &box_1)
    {
        x = Interval<T>(box_0.x, box_1.x);
        y = Interval<T>(box_0.y, box_1.y);
        z = Interval<T>(box_0.z, box_1.z);
    }

    constexpr const Interval<T> &AxisInterval(std::size_t i) const
    {
        assert(0 <= i && i <= 2 && "AxisAlignedBoundingBox::AxisInterval requires argument between [0, 2]!");
        if (i == 0) return x;
        if (i == 1) return y;
        return z;
    }

    std::size_t LongestAxis() const
    {
        if (x.Size() > y.Size()) return x.Size() > z.Size() ? 0 : 2;
        else return y.Size() > z.Size() ? 1 : 2;
    }

    // NOTE: typing here might change later
    bool Hit(const dRay &ray, dInterval ray_interval) const
    {
        const dPoint3 ray_origin = ray.origin;
        const dVector3 ray_direction = ray.direction;

        for (auto axis{0zu}; axis < 3; ++axis)
        {
            const dInterval &axis_interval = AxisInterval(axis);
            const f64 axis_direction_inverse = 1.0 / ray_direction[axis];

            auto t0 = (axis_interval.lower - ray_origin[axis]) * axis_direction_inverse;
            auto t1 = (axis_interval.upper - ray_origin[axis]) * axis_direction_inverse;

            if (t0 < t1)
            {
                if (t0 > ray_interval.lower) ray_interval.lower = t0;
                if (t1 < ray_interval.upper) ray_interval.upper = t1;
            }
            else
            {
                if (t1 > ray_interval.lower) ray_interval.lower = t1;
                if (t0 < ray_interval.upper) ray_interval.upper = t0;
            }

            if (ray_interval.upper <= ray_interval.lower) return false;
        }

        return true;
    }
};

template <Math::Number T>
using AABB = AxisAlignedBoundingBox<T>;
