#include "config.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

void DrawDebugUI(game_main *g, float dt)
{
    if (!Global_DebugUI) return;

    auto &updateTime = g->UpdateTime;
    auto &renderTime = g->RenderTime;
    auto playerEntity = g->World.PlayerEntity;
    auto lanes = g->World.GenState->LaneSlots;
    ImGui::Text("ms: %.2f", dt * 1000.0f);
    ImGui::Text("FPS: %.5f", 1.0f/dt);
    ImGui::Text("Update time: %dms", updateTime.End - updateTime.Begin);
    ImGui::Text("Render time: %dms", renderTime.End - renderTime.Begin);
    ImGui::Text("Time elapsed: %.2f", g->LevelState.TimeElapsed);
    ImGui::Text("Player min vel: %.2f", g->LevelState.PlayerMinVel);
    ImGui::Text("Player max vel: %.2f", g->LevelState.PlayerMaxVel);
    ImGui::Text("Player vel: %s", ToString(playerEntity->Vel()).c_str());
    ImGui::Text("Checkpoint: %d", g->LevelState.CheckpointNum);
    ImGui::Text("Checkpoint time: %.2f", float(g->LevelState.CurrentCheckpointFrame) / 60.0f);
    ImGui::Text("Lanes: %d|%d|%d|%d|%d", lanes[0], lanes[1], lanes[2], lanes[3], lanes[4]);
	ImGui::Text("Road bounds: %.2f|%.2f", g->World.RoadState.Left, g->World.RoadState.Right);
	
	if (ImGui::CollapsingHeader("Music"))
	{
		ImGui::Text("Timer: %.2fs", g->MusicMaster.Timer);
		ImGui::Text("Beat: %d", g->MusicMaster.Beat);
		ImGui::Text("Beat/2: %d", g->MusicMaster.Beat/2);
		ImGui::Text("Beat/4: %d", g->MusicMaster.Beat/4);
		ImGui::Text("Beat time: %.2fs", g->MusicMaster.BeatTime);
		ImGui::Spacing();
	}
	if (ImGui::CollapsingHeader("Background"))
	{
		auto &instances = g->World.BackgroundState.Instances;
		ImGui::Text("Offset: %s", ToString(g->World.BackgroundState.Offset).c_str());
		ImGui::DragFloat2("Instance 0 offset", &instances[0].Offset[0]);
		ImGui::DragFloat2("Instance 1 offset", &instances[1].Offset[0]);
		ImGui::Spacing();
	}
	if (ImGui::CollapsingHeader("Timers"))
	{
		for (int i = 0; i < Global_ProfileTimersCount; i++)
		{
			auto &t = Global_ProfileTimers[i];
			ImGui::Text("%s timer: %dms", t.Name, t.Delta());
		}
		ImGui::Spacing();
	}
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Checkbox("FreeCam", &Global_Camera_FreeCam);
        ImGui::SliderFloat("Offset Y", &Global_Camera_OffsetY, -50.0f, 50.0f, "%.2f");
        ImGui::SliderFloat("Offset Z", &Global_Camera_OffsetZ, -50.0f, 50.0f, "%.2f");
        ImGui::SliderFloat("Look Offset Y", &Global_Camera_LookYOffset, -50.0f, 50.0f, "%.2f");
        ImGui::SliderFloat("Look Offset Z", &Global_Camera_LookZOffset, -50.0f, 50.0f, "%.2f");
        ImGui::SliderFloat("FOV", &Global_Camera_FieldOfView, 20.0f, 200.0f, "%.2f");
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
		ImGui::Text("Renderables: %lu", g->RenderState.FrameSolidRenderables.size() + g->RenderState.FrameTransparentRenderables.size());
		ImGui::Spacing();
        ImGui::Checkbox("PostFX", &Global_Renderer_DoPostFX);
		ImGui::SliderFloat("Bend radius", &g->RenderState.BendRadius, 0, 1000.0f, "%.2f");

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
