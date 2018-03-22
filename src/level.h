#ifndef DRAFT_LEVEL_H
#define DRAFT_LEVEL_H

#define CRYSTAL_COLOR     IntColor(FirstPalette.Colors[1])

#define FRAME_SECONDS(s) (s * 60)

enum level_command_type
{
	LevelCommand_Enable,
	LevelCommand_Disable,
	LevelCommand_AddIntroText,
	LevelCommand_ShipColor,
	LevelCommand_SpawnCheckpoint,
	LevelCommand_SpawnFinish,
    LevelCommand_RoadTangent,
    LevelCommand_PlayTrack,
    LevelCommand_StopTrack,
};

struct level_command
{
	level_command_type Type;

	union
	{
		struct
		{
			size_t ColorHash;
			const char *Text;
		} AddIntroText;

		size_t Hash;
	};
};

struct level_frame
{
	std::vector<level_command> Commands;
	int Frame;
};

struct level_checkpoint
{
	std::vector<level_frame> Frames;
	float PlayerMinVel;
	float PlayerMaxVel;
	int CurrentFrameIndex = 0;
};

struct level
{
    std::vector<level_command> StatsScreenCommands;
	std::vector<level_checkpoint> Checkpoints;
	std::string Name;
	std::string SongName;
	song *Song;
    bool HasRunStatsScreenCommands = false;
};

#endif
