#include "imgui.h"

#include <filesystem>
#include <limits>
#include <ranges>
#include <thread>

#include "Camera.hpp"
#include "Renderer/Renderer.hpp"

#include "MyGui/Wrappers.hpp"

#include "Math/Vector.hpp"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Timer.h"

#include "Util/Aliases.hpp"
#include "Util/Log.hpp"

namespace
{

using FOV       = f32;
using NearPlane = f32;
using FarPlane  = f32;

} // namespace

class RayTracerLayer : public Walnut::Layer
{
private:
    Renderer renderer_{};
    Camera   camera_;
    Scene    scene_;
    u32      viewport_width_{ 0 };
    u32      viewport_height_{ 0 };
    f32      last_render_time_{ 0.f };

public:
    RayTracerLayer() : camera_{ FOV{ 45.0f }, NearPlane{ 0.1f }, FarPlane{ 100.0f } }
    {

        if (!std::filesystem::exists("imgui.ini"))
            ImGui::LoadIniSettingsFromDisk("DefaultLayout.ini");

        auto &pink_sphere     = scene_.materials.emplace_back();
        pink_sphere.albedo    = fVector3{ 1.f, 0.f, 1.f };
        pink_sphere.roughness = 1.00f;

        auto &yellow_sphere     = scene_.materials.emplace_back();
        yellow_sphere.albedo    = fVector3{ 1.f, 1.0f, 0.0f };
        yellow_sphere.roughness = 0.05f;

        auto &orange_light          = scene_.materials.emplace_back();
        orange_light.albedo         = { 0.8f, 0.5f, 0.2f };
        orange_light.roughness      = 0.05f;
        orange_light.emission_color = orange_light.albedo;
        orange_light.emission_power = 2.f;

        scene_.spheres.push_back({ .position{ 0.f, -101.f, 0.f }, .radius = 100.f, .material_index = 0 });
        scene_.spheres.push_back({ .position{ 0.f }, .radius = 1.0f, .material_index = 1 });
        scene_.spheres.push_back({ .position{ 3.f, 0.f, 0.f }, .radius = 1.0f, .material_index = 2 });

        scene_.background = fVector3{ 0.576f, 0.879f, 0.869f };
    }

    virtual void OnUpdate(f32 timestamp) override
    {
        if (camera_.OnUpdate(timestamp))
            renderer_.ResetFrameIndex();
        renderer_.OnUpdate(timestamp);
    }

    virtual void OnUIRender() override
    {
        bool reset_frame_index{ false };

        ImGui::Dummy(ImVec2{ 0.f, 10.f });

        ImGui::Begin("Scene");
        if (ImGui::Button("Add Sphere"))
        {
            AddSphere();
            reset_frame_index = true;
        }

        MyGui::Padding(ImVec2{ 0.f, 10.f });

        if (ImGui::ColorEdit3("Background", Math::ValuePointer(scene_.background)))
            reset_frame_index = true;

        MyGui::Padding(ImVec2{ 0.f, 10.f });

        ImGui::Text("Spheres:");
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

        ImGui::Text("Materials:");
        for (auto [id, material] : scene_.materials | std::views::enumerate)
        {
            ImGui::PushID(id);

            bool is_metallic = material.metallic != 0;
            if (ImGui::Checkbox("Metallic", &is_metallic))
            {
                material.metallic = is_metallic ? 1 : 0;
                reset_frame_index = true;
            }

            if (ImGui::ColorEdit3("Albedo", Math::ValuePointer(material.albedo))
                || ImGui::DragFloat("Roughness", &material.roughness, 0.01f, 0.f, 1.f)
                || ImGui::ColorEdit3("Emission Color", Math::ValuePointer(material.emission_color))
                || ImGui::DragFloat("Emission Power", &material.emission_power, 0.01f, 0.f,
                                    std::numeric_limits<f32>::max()))
            {
                reset_frame_index = true;
            }

            MyGui::Padding(ImVec2{ 0.f, 10.f });

            ImGui::PopID();
        }

        ImGui::End(); // Scene

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        viewport_width_  = ImGui::GetContentRegionAvail().x;
        viewport_height_ = ImGui::GetContentRegionAvail().y;

        if (viewport_width_ > 0 && viewport_height_ > 0)
            RenderViewport();

        if (auto descriptor_set = renderer_.GetDescriptorSet();
            descriptor_set != VK_NULL_HANDLE && viewport_width_ > 0 && viewport_height_ > 0)
        {
            ImGui::Image(descriptor_set,
                         { static_cast<f32>(viewport_width_), static_cast<f32>(viewport_height_) },
                         ImVec2(0, 1), ImVec2(1, 0));
        }

        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        ImGui::Begin("Settings");
        ImGui::Text("Last Render: %.3fms", last_render_time_);

        ImGui::Checkbox("Accumulate", &renderer_.GetSettings().accumulate);

        if (ImGui::Button("Reset"))
            renderer_.ResetFrameIndex();

        ImGui::End(); // Settings

        // NOTE: keep UI above this code
        if (reset_frame_index)
            renderer_.ResetFrameIndex();
    }

    void RenderViewport()
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
    Walnut::ApplicationSpecification spec{
        .Name            = "Realtime Ray Tracer",
        .EnableDebugInfo = true,
        .CustomTitleBar  = true,
    };

    Walnut::Application *app = new Walnut::Application(spec);
    app->PushLayer<RayTracerLayer>();
    if (spec.CustomTitleBar)
    {
        app->SetMenubarCallback(
            [app, spec]()
            {
                if (ImGui::BeginMenu(spec.Name.c_str()))
                {
                    if (ImGui::MenuItem("Exit"))
                    {
                        app->Close();
                    }
                    ImGui::EndMenu();
                }
            });
    }
    return app;
}
