#include "imgui.h"

#include "Renderer.hpp"

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

} // namespace

class ExampleLayer : public Walnut::Layer
{
private:
    Renderer renderer_{};
    u32 viewport_width_{ 0 };
    u32 viewport_height_{ 0 };
    f32 last_render_time_{ 0.f };

public:
    virtual void OnUIRender() override
    {
        ImGui::Begin("Settings");
        ImGui::Text("Last Render: %.3fms", last_render_time_);

        // if (ImGui::Button("Render"))
        // {
        //     Render();
        // }

        ImGui::Text("Sphere Colors:");
        if (auto temp = renderer_.GetSphereColors().r; ImGui::SliderFloat("Red", &temp, 0.0f, 1.0f))
        {
            UpdateColorValue(temp, RGB::Red);
        }
        if (auto temp = renderer_.GetSphereColors().g; ImGui::SliderFloat("Green", &temp, 0.0f, 1.0f))
        {
            UpdateColorValue(temp, RGB::Green);
        }
        if (auto temp = renderer_.GetSphereColors().b; ImGui::SliderFloat("Blue", &temp, 0.0f, 1.0f))
        {
            UpdateColorValue(temp, RGB::Blue);
        }

        ImGui::Text("Light Direction:");
        if (auto temp = renderer_.GetLightDirection().x; ImGui::SliderFloat("X", &temp, -1.0f, 1.0f))
        {
            UpdateLightDirection(temp, Direction::X);
        }
        if (auto temp = renderer_.GetLightDirection().y; ImGui::SliderFloat("Y", &temp, -1.0f, 1.0f))
        {
            UpdateLightDirection(temp, Direction::Y);
        }
        if (auto temp = renderer_.GetLightDirection().z; ImGui::SliderFloat("Z", &temp, -1.0f, 1.0f))
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
        renderer_.Render();

        last_render_time_ = timer.ElapsedMillis();
    }

private:
    void UpdateColorValue(f32 new_value, RGB channel)
    {
        switch (channel)
        {
            case RGB::Red: renderer_.SetRed(new_value); break;
            case RGB::Green: renderer_.SetGreen(new_value); break;
            case RGB::Blue: renderer_.SetBlue(new_value); break;
        }
    }

    void UpdateLightDirection(f32 new_value, Direction direction)
    {
        switch (direction)
        {
            case Direction::X: renderer_.SetX(new_value); break;
            case Direction::Y: renderer_.SetY(new_value); break;
            case Direction::Z: renderer_.SetZ(new_value); break;
        }
    }
};

Walnut::Application *Walnut::CreateApplication(int argc, char **argv)
{
    Walnut::ApplicationSpecification spec;
    spec.Name = "Realtime Ray Tracer";

    Walnut::Application *app = new Walnut::Application(spec);
    app->PushLayer<ExampleLayer>();
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
