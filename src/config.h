#ifndef DRAFT_CONFIG_H
#define DRAFT_CONFIG_H

static bool  Global_DebugUI = false;

static bool  Global_Camera_FreeCam = false;
static float Global_Camera_OffsetY = -3.0f;
static float Global_Camera_OffsetZ = 3.0f;
static float Global_Camera_LookYOffset = 10.0f;
static float Global_Camera_LookZOffset = 0.0f;
static float Global_Camera_FieldOfView = 75.0f;

static bool  Global_Collision_DrawCollider = false;

static float Global_Game_TimeSpeed          = 1.0f;
static float Global_Game_BoostVelocity      = 75.0f;
static float Global_Game_TrailRecordTimer   = 0.016f;
static float Global_Game_DraftChargeTime    = 0.1f;
static float Global_Game_ExplosionLifeTime  = 1.0f;
static float Global_Game_ExplosionLightTime = 1.0f;
static float Global_Game_WallRaiseTime      = 0.5f;
static float Global_Game_WallStartOffset    = -5.0f;
static float Global_Game_WallHeight         = 10.0f;

static bool  Global_Renderer_DoPostFX = true;
static bool  Global_Renderer_BloomEnabled = true;
static float Global_Renderer_BloomBlurOffset = 1.8f;
static float Global_Renderer_FogStart = 500.0f;
static float Global_Renderer_FogEnd = 1000.0f;

struct resolution
{
	int Width;
	int Height;
};

static resolution Global_Resolutions[] = {
	{1920, 1080},
	{1600, 900},
	{1366, 768},
	{1280, 720},
};

static const char *Global_ResolutionTexts[] = {
	"1920x1080",
	"1600x900",
	"1366x768",
	"1280x720",
};

#endif
