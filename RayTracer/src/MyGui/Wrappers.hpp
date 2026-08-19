#include <imgui.h>

namespace MyGui
{
// NOTE: this function sucks
inline bool DragFloat3Alias(const char *label, float values[3], const char *aliases[3],
                            float drag_speed = 1.f, float drag_min = 0.f, float drag_max = 0.f)
{
    ImGui::PushID(label);
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);

    bool changed = false;

    float item_width = (ImGui::CalcItemWidth() - 2.f * ImGui::GetStyle().ItemSpacing.x / 3.f);

    for (auto i{ 0uz }; i < 3; ++i)
    {
        ImGui::PushID(i);
        ImGui::SetNextItemWidth(item_width);

        if (i > 0)
        {
            ImGui::SameLine();
        }
        if (ImGui::DragFloat(aliases[i], &values[i], drag_speed, drag_min, drag_max))
        {
            changed = true;
        }

        ImGui::PopID();
    }

    ImGui::EndGroup();
    ImGui::PopID();

    return changed;
}
} // namespace MyGui
