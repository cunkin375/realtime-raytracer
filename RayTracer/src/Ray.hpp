#pragma once

#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

struct Ray
{
public:
    fPoint3 origin;
    fVector3 direction;
    f32 time;

public:
    Ray() = default;

    Ray(const fPoint3 &_origin, const fVector3 &_direction, f32 _time)
        : origin{_origin}, direction{_direction}, time{_time}
    {
    }

    Ray(const fPoint3 &_origin, const fVector3 &_direction) : origin{_origin}, direction{_direction}, time{0}
    {
    }

    fPoint3 At(f32 scalar) const { return origin + scalar * direction; }
};

struct dRay
{
public:
    dPoint3 origin;
    dVector3 direction;
    f64 time;

public:
    dRay() = default;

    dRay(const dPoint3 &_origin, const dVector3 &_direction, f64 _time)
        : origin{_origin}, direction{_direction}, time{_time}
    {
    }

    dRay(const dPoint3 &_origin, const dVector3 &_direction) : origin{_origin}, direction{_direction}, time{0}
    {
    }

    dPoint3 At(f64 t) const { return origin + t * direction; }
};
