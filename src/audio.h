#ifndef DRAFT_AUDIO_H
#define DRAFT_AUDIO_H

enum audio_clip_type
{
	AudioClipType_Sound,
	AudioClipType_Stream,
};

struct audio_clip
{
	audio_clip_type Type;
	int NumBuffers = 1;
	ALuint *Buffers;
	int SampleCount;

	SF_INFO Info;
	SNDFILE *SndFile;
};

struct song
{
	int Bpm;
	std::vector<audio_clip *> Tracks;
	std::vector<std::string> Files;
	std::unordered_map<std::string, int> Names;
};

struct next_beat_callback
{
	typedef void (func)(void *);

	func *Func;
	void *Arg;
	int Divisor;
};

enum music_master_message_type
{
	MusicMasterMessageType_Tick,
	MusicMasterMessageType_PlayTrack,
	MusicMasterMessageType_StopTrack,
	MusicMasterMessageType_OnNextBeat,
	MusicMasterMessageType_SetPitch,
};

struct music_master_message
{
	music_master_message_type Type;
	union
	{
		struct {
			float DeltaTime;
		} Tick;

		struct {
			int Track;
		} PlayTrack;

		struct {
			int Track;
		} StopTrack;

		next_beat_callback NextBeatCallback;

		struct {
			float Pitch;
		} SetPitch;
	};

	music_master_message() {}
};

struct audio_source;

struct music_master
{
	memory_arena Arena;
	std::thread Thread;
	std::mutex Mutex;
	std::condition_variable ConditionVar;
	std::queue<music_master_message> Queue;
	std::atomic_bool StepBeat = false;

	null_array<next_beat_callback *, 32> NextBeatCallbacks;
	generic_pool<next_beat_callback> NextBeatCallbackPool;

	std::vector<audio_source *> Sources;
	std::vector<bool> SourcesState;
	song *Song;
	float Timer = 0;
	float BeatTime = 0;
	float ElapsedTime = 0;
	int Bpm = 0;
	int Beat = 0;
	bool StopLoop = false;
};

#endif