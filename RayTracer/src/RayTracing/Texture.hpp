#pragma once

#include <variant>

#include "Math/Interval.hpp"
#include "Math/Vector.hpp"
#include "Util/Aliases.hpp"

#include "STB_Image.hpp"

class Texture;

namespace Textures
{

class SolidColor
{
private:
    dColor albedo_;

public:
    SolidColor() = default;

    SolidColor(const dColor &albedo) : albedo_{albedo} {}

    SolidColor(f64 red, f64 green, f64 blue) : SolidColor{dColor{red, green, blue}} {}

    dColor Value(f64 u, f64 v, const dPoint3 &p) const noexcept { return albedo_; }
};

template <typename T, typename U = T>
class Checker
{
private:
    f64 inverse_scale_;
    T even_texture_;
    U odd_texture_;

public:
    Checker() = default;

    Checker(f64 scale, const T &even_texture, const U &odd_texture)
        : inverse_scale_{1.0 / scale}, even_texture_{even_texture}, odd_texture_{odd_texture}
    {
    }

    dColor Value(f64 u, f64 v, const dPoint3 &point) const noexcept
    {
        auto x_integer = i32(std::floor(inverse_scale_ * point.x));
        auto y_integer = i32(std::floor(inverse_scale_ * point.y));
        auto z_integer = i32(std::floor(inverse_scale_ * point.z));

        bool is_even = (x_integer + y_integer + z_integer) % 2 == 0;

        return is_even ? even_texture_.Value(u, v, point) : odd_texture_.Value(u, v, point);
    }
};

class ImageTexture
{
private:
    Image image;

public:
    ImageTexture(const char *filename) : image{filename} {}

    dColor Value(f64 u, f64 v, const dPoint3 &point) const
    {
        // cyan color for debugging missing texture
        if (image.Height() <= 0) return dColor{0, 1, 1};

        // clamp texture coords to [0, 1] * [1, 0]
        u = Math::Interval<f64>{0, 1}.Clamp(u);
        v = 1.0 - Math::Interval<f64>{0, 1}.Clamp(v);

        auto i = i32(u * image.Width());
        auto j = i32(v * image.Height());
        auto pixel = image.PixelData(i, j);

        auto color_scale = 1.0 / 255.0;

        // d pointer arithmetic based on the color scale to visit rgb pixel information in image
        return dColor{color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[3]};
    }
};

using Variant = std::variant<SolidColor, Checker<SolidColor>>;

} // namespace Textures

class Texture
{
private:
    Textures::Variant data_;

public:
    Texture() = default;

    template <typename T>
    Texture(T &&data) : data_{std::forward<T>(data)}
    {
    }

    // NOTE: this might not be needed
    Texture(const dColor &data) : data_{Textures::SolidColor{data}} {}

    dColor Value(f64 u, f64 v, dPoint3 point) const
    {
        return std::visit([&](const auto &texture) { return texture.Value(u, v, point); }, data_);
    }
};
