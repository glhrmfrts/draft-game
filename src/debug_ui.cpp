#include "config.h"

void DrawDebugUI(float DeltaTime)
{
    if (!Global_DebugUI) return;

    ImGui::Text("FPS: %.5f", 1.0f/DeltaTime);
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Checkbox("FreeCam", &Global_Camera_FreeCam);
    }
    if (ImGui::CollapsingHeader("Collision"))
    {
        ImGui::Checkbox("Draw Bounds", &Global_Collision_DrawBounds);
    }
    if (ImGui::CollapsingHeader("Game"))
    {
        if (ImGui::TreeNode("Trail"))
        {
            ImGui::SliderFloat("Record Timer", &Global_Game_TrailRecordTimer, 0.0f, 4.0f, "%.3f");
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Renderer"))
    {
        ImGui::Checkbox("PostFX", &Global_Renderer_DoPostFX);

        if (ImGui::TreeNode("Bloom"))
        {
            ImGui::SliderFloat("Blur Offset", &Global_Renderer_BloomBlurOffset, 0.0f, 10.0f, "%.2f");
            ImGui::TreePop();
        }
    }
}
