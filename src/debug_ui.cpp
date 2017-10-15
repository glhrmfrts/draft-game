#include "config.h"

void DrawDebugUI(game_state &g, float dt)
{
    if (!Global_DebugUI) return;

    auto &updateTime = g.UpdateTime;
    auto &renderTime = g.RenderTime;
    auto playerEntity = g.PlayerEntity;
    ImGui::Text("ms: %.2f", dt * 1000.0f);
    ImGui::Text("FPS: %.5f", 1.0f/dt);
    ImGui::Text("Update time: %dms", updateTime.End - updateTime.Begin);
    ImGui::Text("Render time: %dms", renderTime.End - renderTime.Begin);
    ImGui::Text("Player max vel: %.2f", g.LevelMode.PlayerMaxVel);
    ImGui::Text("Player vel: %s", ToString(playerEntity->Vel()).c_str());

    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Checkbox("FreeCam", &Global_Camera_FreeCam);
        ImGui::SliderFloat("Offset Y", &Global_Camera_OffsetY, -50.0f, 50.0f, "%.2f");
        ImGui::SliderFloat("Offset Z", &Global_Camera_OffsetZ, -50.0f, 50.0f, "%.2f");
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("Collision"))
    {
        ImGui::Checkbox("Draw Bounds", &Global_Collision_DrawCollider);
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("Game"))
    {
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("Renderer"))
    {
		ImGui::Text("Renderables: %lu", g.RenderState.FrameSolidRenderables.size() + g.RenderState.FrameTransparentRenderables.size());
		ImGui::Spacing();
        ImGui::Checkbox("PostFX", &Global_Renderer_DoPostFX);

        if (ImGui::TreeNode("Bloom"))
        {
			ImGui::Checkbox("Enabled", &Global_Renderer_BloomEnabled);
            ImGui::SliderFloat("Blur Offset", &Global_Renderer_BloomBlurOffset, 0.0f, 10.0f, "%.2f");
            ImGui::TreePop();
        }
		if (ImGui::TreeNode("Fog"))
		{
			ImGui::SliderFloat("Start", &Global_Renderer_FogStart, 0.0f, 500.0f, "%.2f");
			ImGui::SliderFloat("End", &Global_Renderer_FogEnd, 0.0f, 500.0f, "%.2f");
			ImGui::TreePop();
		}
        ImGui::Spacing();
    }
}
