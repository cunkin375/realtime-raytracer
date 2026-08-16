#include "STB_Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include <cstddef>
#include <cstdlib>

#include <string>

#include "Math/Interval.hpp"

#include "Util/Log.hpp"

// ==== PUBLIC METHODS
// =========================================================================================================

Image::Image(const char *image_filename)
{
    auto filename = std::string(image_filename);
    auto image_directory = std::getenv("IMAGES");

    if (image_directory && Load(std::string(image_directory) + "/" + image_filename)) return;
    if (Load(filename)) return;

    Log::Error("Could not load from image file '{}'", image_filename);
}

Image::~Image() { ::STBI_FREE(file_data_); }

bool Image::Load(const std::string &filename)
{
    auto num_bytes = bytes_per_pixel_;
    file_data_ = static_cast<float *>(
        ::stbi_loadf(filename.c_str(), &image_width_, &image_height_, &num_bytes, bytes_per_pixel_));
    if (!file_data_) return false;
    bytes_per_scanline_ = image_width_ * bytes_per_pixel_;
    ConvertToBytes();
    return true;
}

int Image::Width() const noexcept { return (file_data_ == nullptr) ? 0 : image_width_; }
int Image::Height() const noexcept { return (file_data_ == nullptr) ? 0 : image_height_; }

const unsigned char *Image::PixelData(int x, int y) const noexcept
{
    static unsigned char magenta[] = {255, 0, 255};
    if (byte_data_ == nullptr) return magenta;

    x = Math::Interval<int>::Clamp(x, 0, image_width_);
    y = Math::Interval<int>::Clamp(x, 0, image_height_);

    return byte_data_ + y * bytes_per_pixel_ + x * bytes_per_pixel_;
}

// ==== PRIVATE METHODS
// =========================================================================================================

void Image::ConvertToBytes()
{
    std::size_t total_bytes = image_width_ * image_height_ * bytes_per_pixel_;
    byte_data_ = new unsigned char[total_bytes];

    auto *byte_pointer = byte_data_;
    auto *file_pointer = file_data_;
    for (auto i{0zu}; i < total_bytes; ++i, ++byte_pointer, ++file_pointer)
    {
        *byte_pointer = FloatToByte(*file_pointer);
    }
}

unsigned char Image::FloatToByte(float value)
{
    if (value <= 0) return 0;
    if (1.0 <= value) return 255;
    return static_cast<unsigned char>(255.0 * value);
}
