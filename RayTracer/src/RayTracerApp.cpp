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
        scene_.materials.push_back({ .albedo{ 1.f, 0.f, 1.f }, .roughness = 0.15f });
        scene_.materials.push_back({ .albedo{ 1.f, 1.0f, 0.0f }, .roughness = 0.05f });

        scene_.spheres.push_back({ .position{ 0.f, -101.f, 0.f }, .radius = 100.f, .material_index = 0 });
        scene_.spheres.push_back({ .position{ 0.f }, .radius = 1.0f, .material_index = 1 });
    }

    virtual void OnUpdate(f32 timestamp) override
    {
        if (camera_.OnUpdate(timestamp))
            renderer_.ResetFrameIndex();
    }

    virtual void OnUIRender() override
    {
        bool reset_frame_index{ false };

        ImGui::Dummy(ImVec2{ 0.f, 10.f });

        ImGui::Begin("Scene");
        ImGui::Text("Last Render: %.3fms", last_render_time_);

        if (ImGui::Button("Add Sphere"))
        {
            AddSphere();
            reset_frame_index = true;
        }

        MyGui::Padding(ImVec2{ 0.f, 10.f });

        // Spheres
        for (auto [id, sphere] : scene_.spheres | std::views::enumerate)
        {
            ImGui::PushID(id);

            if (ImGui::DragFloat3("Position", Math::ValuePointer(sphere.position), 0.01f)
                || ImGui::DragFloat("Radius", &sphere.radius, 0.1f)
                || ImGui::DragInt("Material", &sphere.material_index, 1, 0,
                                  static_cast<int>(scene_.materials.size() - 1)))
            {
                reset_frame_index = true;
            }

            MyGui::Padding(ImVec2{ 0.f, 10.f });

            ImGui::PopID();
        }

        // Materials
        for (auto [id, material] : scene_.materials | std::views::enumerate)
        {
            ImGui::PushID(id);

            if (ImGui::ColorEdit3("Albedo", Math::ValuePointer(material.albedo))
                || ImGui::DragFloat("Roughness", &material.roughness, 0.01f, 0.f, 1.f)
                || ImGui::DragFloat("Metallic", &material.metallic, 0.01f, 0.f, 1.f))
            {
                reset_frame_index = true;
            }

            ImGui::PopID();
        }

        MyGui::Padding(ImVec2{ 0.f, 10.f });
        if (ImGui::DragFloat3("Light Direction", Math::ValuePointer(scene_.light_direction), 0.01f, -1.f, 1.f))
            reset_frame_index = true;

        ImGui::End(); // Scene

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        viewport_width_ = ImGui::GetContentRegionAvail().x;
        viewport_height_ = ImGui::GetContentRegionAvail().y;

        auto image = renderer_.GetFinalImage();
        if (image)
        {
            ImGui::Image(image->GetDescriptorSet(),
                         { static_cast<f32>(image->GetWidth()), static_cast<f32>(image->GetHeight()) },
                         ImVec2(0, 1), ImVec2(1, 0));
        }

        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        ImGui::Begin("Settings");
        ImGui::Checkbox("Accumulate", &renderer_.GetSettings().accumulate);
        if (ImGui::Button("Reset"))
            renderer_.ResetFrameIndex();
        ImGui::End(); // Settings

        // NOTE: keep UI above this code
        if (reset_frame_index)
            renderer_.ResetFrameIndex();

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
    void AddSphere() { scene_.spheres.push_back({ .position{ 0.f }, .radius = 1.0f, .material_index = 0 }); }
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
