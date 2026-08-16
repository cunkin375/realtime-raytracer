#include "imgui.h"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Math/Random.hpp"
#include "Util/Aliases.hpp"

class ExampleLayer : public Walnut::Layer
{
private:
    std::shared_ptr<Walnut::Image> image_{};
    u32 *image_data_ = nullptr;
    u32 viewport_width_{ 0 };
    u32 viewport_height_{ 0 };
    f32 last_render_time_{ 0.f };

public:
    virtual void OnUIRender() override
    {
        ImGui::Begin("Settings");
        ImGui::Text("Last Render: %.3fms", last_render_time_);
        if (ImGui::Button("Render")) { Render(); }
        ImGui::End(); // Settings

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        viewport_width_ = ImGui::GetContentRegionAvail().x;
        viewport_height_ = ImGui::GetContentRegionAvail().y;

        if (image_)
            ImGui::Image(image_->GetDescriptorSet(),
                         { static_cast<f32>(image_->GetWidth()), static_cast<f32>(image_->GetHeight()) });

        ImGui::End(); // Viewport
        ImGui::PopStyleVar();

        Render();
    }

    void Render()
    {
        auto timer = Walnut::Timer{};
        if (!image_ || viewport_width_ != image_->GetWidth() || viewport_height_ != image_->GetHeight())
        {
            image_ =
                std::make_shared<Walnut::Image>(viewport_width_, viewport_height_, Walnut::ImageFormat::RGBA);
            delete[] image_data_;
            image_data_ = new u32[viewport_width_ * viewport_height_];
        }

        for (auto i{ 0zu }; i < viewport_width_ * viewport_height_; ++i)
        {
            image_data_[i] = Math::Rand::GenerateRandomNumber<u32>();
            //                   A B G R
            image_data_[i] |= 0xff000000;
        }

        image_->SetData(image_data_);

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
                if (ImGui::MenuItem("Exit")) { app->Close(); }
                ImGui::EndMenu();
            }
        });
    return app;
}
