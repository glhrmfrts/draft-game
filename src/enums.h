#ifndef DRAFT_ENUMS_H
#define DRAFT_ENUMS_H

enum action_type
{
    Action_camHorizontal,
    Action_camVertical,
    Action_horizontal,
    Action_vertical,
    Action_boost,
	Action_debugUI,
    Action_debugPause,
    Action_select,
    Action_pause,
    Action_count,
};

enum asset_completion
{
    AssetCompletion_Incomplete,
    AssetCompletion_ThreadSafe,
    AssetCompletion_ThreadUnsafe,
	AssetCompletion_Done,
};

enum asset_entry_type
{
    AssetEntryType_Texture,
	AssetEntryType_Font,
	AssetEntryType_Shader,
	AssetEntryType_Sound,
	AssetEntryType_Stream,
	AssetEntryType_Song,
	AssetEntryType_OptionsLoad,
	AssetEntryType_OptionsSave,
    AssetEntryType_Level,
	AssetEntryType_Mesh,
    AssetEntryType_MaterialLib,
};

enum audio_clip_type
{
	AudioClipType_Sound,
	AudioClipType_Stream,
};

enum camera_type
{
    Camera_orthographic,
    Camera_perspective,
};

enum checkpoint_state
{
    CheckpointState_Initial,
    CheckpointState_Drafted,
    CheckpointState_Active,
};

enum collider_type
{
    ColliderType_Crystal,
    ColliderType_Ship,
    ColliderType_TrailPiece,
    ColliderType_Asteroid,
	ColliderType_EnemySkull,
};

enum color_texture_type
{
    ColorTextureFlag_SurfaceReflect,
    ColorTextureFlag_Emit,
    ColorTextureFlag_Count,
};

enum entity_type
{
	EntityType_Invalid,
	EntityType_Crystal,
	EntityType_Ship,
	EntityType_Explosion,
	EntityType_Powerup,
	EntityType_Asteroid,
	EntityType_Checkpoint,
	EntityType_Finish,
	EntityType_EnemySkull,
	EntityType_MAX,
};

// @TODO: this will probably only work for linux
enum game_controller_axis_id
{
    Axis_Invalid = -1,
    Axis_LeftX = 0,
    Axis_LeftY = 1,
    Axis_LeftTrigger = 2,
    Axis_RightTrigger = 5,
    Axis_RightX = 3,
    Axis_RightY = 4,
};
enum game_controller_button_id
{
    Button_Invalid = -1,

#ifdef _WIN32
	XboxButton_A = 10,
	XboxButton_B = 11,
	XboxButton_X = 12,
	XboxButton_Y = 13,
	XboxButton_Left = 8,
	XboxButton_Right = 9,
	XboxButton_Start = 654,
#else
    XboxButton_A = 0,
    XboxButton_B = 1,
    XboxButton_X = 2,
    XboxButton_Y = 3,
    XboxButton_Left = 4,
    XboxButton_Right = 5,
    XboxButton_Back = 6,
    XboxButton_Start = 7,
#endif
};

enum game_state
{
	GameState_LoadingScreen,
    GameState_Level,
    GameState_Menu,
};

enum gameplay_state
{
    GameplayState_Playing,
    GameplayState_Paused,
    GameplayState_GameOver,
    GameplayState_Stats,
    GameplayState_ChangingLevel,
};

enum gen_type
{
    GenType_Crystal,
    GenType_Ship,
    GenType_RedShip,
    GenType_Asteroid,
    GenType_SideTrail,
    GenType_RandomGeometry,
	GenType_EnemySkull,
    GenType_MAX,
};

enum level_command_type
{
	LevelCommand_Enable,
	LevelCommand_Disable,
	LevelCommand_AddFlags,
	LevelCommand_RemoveFlags,
	LevelCommand_AddIntroText,
	LevelCommand_ShipColor,
	LevelCommand_SpawnCheckpoint,
	LevelCommand_SpawnFinish,
    LevelCommand_RoadTangent,
	LevelCommand_RoadChange,
    LevelCommand_PlayTrack,
    LevelCommand_StopTrack,
	LevelCommand_SetEntityClip,
};

enum menu_item_type
{
    MenuItemType_Text,
	MenuItemType_SliderFloat,
	MenuItemType_Switch,
	MenuItemType_Options,
};

enum menu_screen
{
	MenuScreen_Main,
	MenuScreen_Play,
	MenuScreen_Opts,
	MenuScreen_OptsGame,
	MenuScreen_OptsAudio,
	MenuScreen_OptsGraphics,
	MenuScreen_Cred,
	MenuScreen_Exit,
};

enum music_master_message_type
{
	MusicMasterMessageType_Tick,
	MusicMasterMessageType_PlayTrack,
	MusicMasterMessageType_StopTrack,
	MusicMasterMessageType_OnNextBeat,
	MusicMasterMessageType_SetPitch,
	MusicMasterMessageType_SetGain,
};

enum road_change
{
	RoadChange_NarrowRight,
	RoadChange_NarrowLeft,
	RoadChange_NarrowCenter,
	RoadChange_WidensLeft,
	RoadChange_WidensRight,
	RoadChange_WidensCenter,
};

enum tween_easing
{
    TweenEasing_Linear,
    TweenEasing_MAX,
};

#endif
