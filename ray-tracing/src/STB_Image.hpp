#pragma once

#include <string>

// Loads image data from the specified file. If the IMAGES environment variable is
// defined, looks only in that directory for the image file. If the image was not found,
// searches for the specified image file first from the current directory, then in the
// images/ subdirectory, then the _parent's_ images/ subdirectory, and then _that_
// parent, on so on, for six levels up. If the image was not loaded successfully,
// width() and height() will return 0.
class Image
{
private:
    unsigned char *byte_data_{};
    float *file_data_{nullptr};
    const int bytes_per_pixel_{3};
    int image_width_{0};
    int image_height_{0};
    int bytes_per_scanline_{0};

public:
    Image() = default;

    Image(const char *image_filename);

    ~Image();

    bool Load(const std::string &filename);

    int Width() const noexcept;
    int Height() const noexcept;

    const unsigned char *PixelData(int x, int y) const noexcept;

private:
    void ConvertToBytes();

    static unsigned char FloatToByte(float value);
};
