#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Walnut/Input/Input.h"

using namespace Walnut;

Camera::Camera(f32 verticalFOV, f32 nearClip, f32 farClip)
    : vertical_fov_(verticalFOV), near_plane_(nearClip), far_plane_(farClip)
{
    forward_direction_ = glm::vec3(0, 0, -1);
    position_ = glm::vec3(0, 0, 6);
}

bool Camera::OnUpdate(f32 ts)
{
    glm::vec2 mousePos = Input::GetMousePosition();
    glm::vec2 delta = (mousePos - last_mouse_position_) * 0.002f;
    last_mouse_position_ = mousePos;

    if (!Input::IsMouseButtonDown(MouseButton::Right))
    {
        Input::SetCursorMode(CursorMode::Normal);
        return false;
    }

    Input::SetCursorMode(CursorMode::Locked);

    bool moved = false;

    constexpr glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
    glm::vec3 rightDirection = glm::cross(forward_direction_, upDirection);

    f32 speed = 5.0f;

    // Movement
    if (Input::IsKeyDown(KeyCode::W))
    {
        position_ += forward_direction_ * speed * ts;
        moved = true;
    }
    else if (Input::IsKeyDown(KeyCode::S))
    {
        position_ -= forward_direction_ * speed * ts;
        moved = true;
    }
    if (Input::IsKeyDown(KeyCode::A))
    {
        position_ -= rightDirection * speed * ts;
        moved = true;
    }
    else if (Input::IsKeyDown(KeyCode::D))
    {
        position_ += rightDirection * speed * ts;
        moved = true;
    }
    if (Input::IsKeyDown(KeyCode::Q))
    {
        position_ -= upDirection * speed * ts;
        moved = true;
    }
    else if (Input::IsKeyDown(KeyCode::E))
    {
        position_ += upDirection * speed * ts;
        moved = true;
    }

    // Rotation
    if (delta.x != 0.0f || delta.y != 0.0f)
    {
        f32 pitchDelta = delta.y * GetRotationSpeed();
        f32 yawDelta = delta.x * GetRotationSpeed();

        glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection),
                                                glm::angleAxis(-yawDelta, glm::vec3(0.f, 1.0f, 0.0f))));
        forward_direction_ = glm::rotate(q, forward_direction_);

        moved = true;
    }

    if (moved)
    {
        RecalculateView();
        RecalculateRayDirections();
    }

    return moved;
}

void Camera::OnResize(u32 width, u32 height)
{
    if (width == viewport_width_ && height == viewport_height_)
        return;

    viewport_width_ = width;
    viewport_height_ = height;

    RecalculateProjection();
    RecalculateRayDirections();
}

f32 Camera::GetRotationSpeed() { return 0.3f; }

void Camera::RecalculateProjection()
{
    projection_ = glm::perspectiveFov(glm::radians(vertical_fov_), (f32)viewport_width_,
                                      (f32)viewport_height_, near_plane_, far_plane_);
    inverse_projection_ = glm::inverse(projection_);
}

void Camera::RecalculateView()
{
    view_ = glm::lookAt(position_, position_ + forward_direction_, glm::vec3(0, 1, 0));
    inverse_view_ = glm::inverse(view_);
}

void Camera::RecalculateRayDirections()
{
    ray_directions_cache_.resize(viewport_width_ * viewport_height_);

    for (u32 y = 0; y < viewport_height_; y++)
    {
        for (u32 x = 0; x < viewport_width_; x++)
        {
            glm::vec2 coord = { (f32)x / (f32)viewport_width_, (f32)y / (f32)viewport_height_ };
            coord = coord * 2.0f - 1.0f; // -1 -> 1

            glm::vec4 target = inverse_projection_ * glm::vec4(coord.x, coord.y, 1, 1);
            glm::vec3 rayDirection = glm::vec3(
                inverse_view_ * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0)); // World space
            ray_directions_cache_[x + y * viewport_width_] = rayDirection;
        }
    }
}
