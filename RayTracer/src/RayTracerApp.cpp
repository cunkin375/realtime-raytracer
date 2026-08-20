#include "imgui.h"

#include <ranges>

#include "Camera.hpp"
#include "Renderer.hpp"

#include "MyGui/Wrappers.hpp"

#include "Math/Vector.hpp"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Timer.h"

#include "Util/Aliases.hpp"

namespace
{
enum class RGB
{
    Red,
    Green,
    Blue,
};

enum class Direction
{
    X,
    Y,
    Z
};

using FOV = f32;
using NearPlane = f32;
using FarPlane = f32;

} // namespace

class RayTracerLayer : public Walnut::Layer
{
private:
    Renderer renderer_{};
    Camera camera_;
    Scene scene_;
    u32 viewport_width_{ 0 };
    u32 viewport_height_{ 0 };
    f32 last_render_time_{ 0.f };

public:
    RayTracerLayer() : camera_(FOV{ 45.0f }, NearPlane{ 0.1f }, FarPlane{ 100.0f })
    {
        scene_.spheres.push_back(
            { .position{ 0.f, -101.f, 0.f }, .radius = 100.f, .material{ .albedo{ 1, 0, 0 } } });
        scene_.spheres.push_back({ .position{ 0.f }, .radius = 1.0f, .material{ .albedo{ 1, 0, 1 } } });
    }

    virtual void OnUpdate(f32 ts) override { camera_.OnUpdate(ts); }

    virtual void OnUIRender() override
    {
        ImGui::Dummy(ImVec2{ 0.f, 10.f });

        ImGui::Begin("Settings");
        ImGui::Text("Last Render: %.3fms", last_render_time_);

        if (ImGui::Button("Add Sphere"))
        {
            AddSphere();
        }

        MyGui::Padding(ImVec2{ 0.f, 10.f });

        for (auto [id, sphere] : scene_.spheres | std::views::enumerate)
        {
            ImGui::PushID(id);

            ImGui::DragFloat3("Position", Math::ValuePointer(sphere.position), 0.01f);
            ImGui::DragFloat("Radius", &sphere.radius, 0.1f);
            ImGui::ColorEdit3("Albedo", Math::ValuePointer(sphere.material.albedo));
            ImGui::DragFloat("Roughness", &sphere.material.roughness, 0.01f);
            ImGui::DragFloat("Metallic", &sphere.material.metallic, 0.01f);

            MyGui::Padding(ImVec2{ 0.f, 10.f });

            ImGui::PopID();
        }

        ImGui::Text("Light Direction:");
        if (auto temp = renderer_.GetLightDirection().x; ImGui::SliderFloat("X", &temp, -1.f, 1.f))
        {
            UpdateLightDirection(temp, Direction::X);
        }
        if (auto temp = renderer_.GetLightDirection().y; ImGui::SliderFloat("Y", &temp, -1.f, 1.f))
        {
            UpdateLightDirection(temp, Direction::Y);
        }
        if (auto temp = renderer_.GetLightDirection().z; ImGui::SliderFloat("Z", &temp, -1.f, 1.f))
        {
            UpdateLightDirection(temp, Direction::Z);
        }

        ImGui::End(); // Settings

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        viewport_width_ = ImGui::GetContentRegionAvail().x;
        viewport_height_ = ImGui::GetContentRegionAvail().y;

        auto image = renderer_.GetFinalImage();
        if (image)
            ImGui::Image(image->GetDescriptorSet(),
                         { static_cast<f32>(image->GetWidth()), static_cast<f32>(image->GetHeight()) },
                         ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        Render();
    }

    void Render()
    {
        auto timer = Walnut::Timer{};

        renderer_.OnResize(viewport_width_, viewport_height_);
        camera_.OnResize(viewport_width_, viewport_height_);
        renderer_.Render(camera_, scene_);

        last_render_time_ = timer.ElapsedMillis();
    }

private:
    void UpdateLightDirection(f32 new_value, Direction direction)
    {
        switch (direction)
        {
            case Direction::X: renderer_.SetX(new_value); break;
            case Direction::Y: renderer_.SetY(new_value); break;
            case Direction::Z: renderer_.SetZ(new_value); break;
        }
    }

    void AddSphere()
    {
        scene_.spheres.push_back({ .position{ 0.f }, .radius = 1.0f, .material{ .albedo{ 1, 0, 1 } } });
    }
};

Walnut::Application *Walnut::CreateApplication(int argc, char **argv)
{
    Walnut::ApplicationSpecification spec;
    spec.Name = "Realtime Ray Tracer";

    Walnut::Application *app = new Walnut::Application(spec);
    app->PushLayer<RayTracerLayer>();
    app->SetMenubarCallback(
        [app]()
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                {
                    app->Close();
                }
                ImGui::EndMenu();
            }
        });
    return app;
}
