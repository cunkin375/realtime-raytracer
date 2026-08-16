#include "Camera.hpp"

// ==== PUBLIC METHODS
// ======================================================================================================

Camera::Camera(f64 _aspect_ratio, std::size_t _image_width)
    : aspect_ratio{_aspect_ratio}, image_width{_image_width}
{
}

Camera::Camera(f64 _aspect_ratio, std::size_t _image_width, std::size_t _samples_per_pixel)
    : aspect_ratio{_aspect_ratio}, image_width{_image_width}, samples_per_pixel{_samples_per_pixel}
{
}

Camera::Camera(f64 _aspect_ratio, std::size_t _image_width, std::size_t _samples_per_pixel, f64 _vertical_fov)
    : aspect_ratio{_aspect_ratio}, image_width{_image_width}, samples_per_pixel{_samples_per_pixel},
      vertical_fov{_vertical_fov}
{
}

// ==== PRIVATE METHODS
// =====================================================================================================

dRay Camera::GetRay(std::size_t i, std::size_t j) const
{
    auto offset = SampleSquare();
    auto pixel_sample =
        pixel_00_location_ + ((i + offset.x) * pixel_delta_u_) + ((j + offset.y) * pixel_delta_v_);
    auto ray_origin = (defocus_angle <= 0) ? camera_center_ : DefocusDiskSample();
    auto ray_direction = pixel_sample - ray_origin;
    auto ray_time = Math::Rand::GenerateRandomNormalizedNumber<f64>();
    return dRay{ray_origin, ray_direction, ray_time};
}

dPoint3 Camera::DefocusDiskSample() const
{
    auto point = dVector2::GenerateRandomUnitVector();
    return camera_center_ + (point.x * defocus_disk_u) + (point.y * defocus_disk_v);
}

dVector3 Camera::SampleSquare() const
{
    using namespace Math;
    return dVector3{Rand::GenerateRandomNormalizedNumber<f64>() - 0.5,
                    Rand::GenerateRandomNormalizedNumber<f64>() - 0.5, 0};
}

void Camera::InitializePass()
{
    image_height_ = std::size_t(image_width / aspect_ratio);
    // image height must at least be 1
    image_height_ = (image_height_ < 1) ? 1 : image_height_;

    pixel_samples_scale_ = 1.0 / samples_per_pixel;

    camera_center_ = look_from;

    // Viewport dimensions
    // f64 focal_length = (look_from - look_at).Magnitude();
    f64 theta = Math::DegreesToRadians(vertical_fov);
    auto h_ratio = std::tan(theta / 2);

    f64 viewport_height = 2.0 * h_ratio * focus_distance;
    f64 viewport_width = viewport_height * (f64(image_width) / image_height_);

    // u, v, w unit basis vectors for camerate coordinate frame
    w = dVector3::UnitVector(look_from - look_at);
    u = dVector3::UnitVector(dVector3::CrossProduct(vertical_up, w));
    v = dVector3::CrossProduct(w, u);

    // Vectors across viewport edges
    auto viewport_u = viewport_width * u;
    auto viewport_v = viewport_height * -v;

    // Horizontal and Vertical unit vectors
    pixel_delta_u_ = viewport_u / image_width;
    pixel_delta_v_ = viewport_v / image_height_;

    // Location of upper left pixel
    auto viewport_upper_left = camera_center_ - (focus_distance * w) - viewport_u / 2 - viewport_v / 2;
    pixel_00_location_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);

    // Camera defocus disk basis vectors
    auto defocus_radius = focus_distance * std::tan(Math::DegreesToRadians(defocus_angle / 2));
    defocus_disk_u = u * defocus_radius;
    defocus_disk_v = v * defocus_radius;
}
