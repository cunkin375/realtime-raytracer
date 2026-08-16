#include "imgui.h"

#include "Renderer.hpp"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Timer.h"

#include "Util/Aliases.hpp"

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
        if (ImGui::Button("Render"))
        {
            Render();
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
                           ImVec2(0, 1), ImVec2(1, 0) );

        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        Render();
    }

    void Render()
    {
        auto timer = Walnut::Timer{};

        f32 aspect_ratio = static_cast<f32>(viewport_width_) / viewport_height_;
        renderer_.OnResize(viewport_width_, viewport_height_);
        renderer_.Render(aspect_ratio);

        last_render_time_ = timer.ElapsedMillis();
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
