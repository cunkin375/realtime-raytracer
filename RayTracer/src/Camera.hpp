#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Util/Aliases.hpp"

class Camera
{
private:
    glm::mat4 projection_{ 1.0f };
    glm::mat4 view_{ 1.0f };
    glm::mat4 inverse_projection_{ 1.0f };
    glm::mat4 inverse_view_{ 1.0f };

    f32 vertical_fov_ = 45.0f;
    f32 near_plane_ = 0.1f;
    f32 far_plane_ = 100.0f;

    glm::vec3 position_{ 0.0f, 0.0f, 0.0f };
    glm::vec3 forward_direction_{ 0.0f, 0.0f, 0.0f };

    std::vector<glm::vec3> ray_directions_cache_;

    glm::vec2 last_mouse_position_{ 0.0f, 0.0f };

    u32 viewport_width_ = 0;
    u32 viewport_height_ = 0;

public:
    Camera(f32 vertical_fov, f32 near_plane, f32 far_plane);

    bool OnUpdate(f32 ts);
    void OnResize(u32 width, u32 height);

    const glm::mat4 &GetProjection() const noexcept { return projection_; }
    const glm::mat4 &GetInverseProjection() const noexcept { return inverse_projection_; }
    const glm::mat4 &GetView() const noexcept { return view_; }
    const glm::mat4 &GetInverseView() const noexcept { return inverse_view_; }

    const glm::vec3 &GetPosition() const noexcept { return position_; }
    const glm::vec3 &GetDirection() const noexcept { return forward_direction_; }

    const std::vector<glm::vec3> &GetRayDirections() const { return ray_directions_cache_; }

    f32 GetRotationSpeed();

private:
    void RecalculateProjection();
    void RecalculateView();
    void RecalculateRayDirections();

};
