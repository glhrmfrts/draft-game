#ifndef DRAFT_CONFIG_H
#define DRAFT_CONFIG_H

static bool  Global_DebugUI = false;

static bool  Global_Camera_FreeCam = false;
static float Global_Camera_OffsetY = -8.0f;
static float Global_Camera_OffsetZ = 2.0f;

static bool  Global_Collision_DrawBounds = false;

static float Global_Game_BoostVelocity = 75.0f;
static float Global_Game_TrailRecordTimer = 0.015f;
static float Global_Game_DraftChargeTime = 1.0f;
static float Global_Game_ExplosionLifeTime = 1.0f;
static float Global_Game_ExplosionLightTime = 1.0f;

static bool  Global_Renderer_DoPostFX = true;
static bool  Global_Renderer_BloomEnabled = true;
static float Global_Renderer_BloomBlurOffset = 1.8f;
static float Global_Renderer_FogStart = 30.0f;
static float Global_Renderer_FogEnd = 500.0f;

#endif
