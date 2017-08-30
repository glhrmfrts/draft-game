#include "config.h"

void DrawDebugUI(float DeltaTime)
{
    if (!Global_DebugUI) return;

    ImGui::Text("FPS: %.5f", 1.0f/DeltaTime / 10.0f);
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Checkbox("FreeCam", &Global_Camera_FreeCam);
    }
    if (ImGui::CollapsingHeader("Collision"))
    {
        ImGui::Checkbox("Draw Bounds", &Global_Collision_DrawBounds);
    }
    if (ImGui::CollapsingHeader("Renderer"))
    {
        ImGui::Checkbox("PostFX", &Global_Renderer_DoPostFX);

        if (ImGui::TreeNode("Bloom"))
        {
            ImGui::SliderInt("Blur Amount", &Global_Renderer_BloomBlurAmount, 0, 10);
            ImGui::SliderFloat("Blur Scale", &Global_Renderer_BloomBlurScale, 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Blur Strength", &Global_Renderer_BloomBlurStrength, 0.0f, 1.0f, "%.3f");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("FXAA"))
        {
            ImGui::SliderInt("Passes", &Global_Renderer_FXAAPasses, 0, 10);
            ImGui::TreePop();
        }
    }
}
