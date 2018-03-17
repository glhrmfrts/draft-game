// Copyright

#define AUDIO_STREAMING_BUFFER_SIZE 48000

static inline ALenum GetFormatFromChannels(int Count)
{
	switch (Count)
	{
	case 1:
		return AL_FORMAT_MONO16;
	case 2:
		return AL_FORMAT_STEREO16;
	default:
		return 0;
	}
}

static bool Stream(audio_source *source, ALuint buffer)
{
	auto clip = source->Clip;
	int size = AUDIO_STREAMING_BUFFER_SIZE / sizeof(short);
	int read = 0;

	static short fileData[AUDIO_STREAMING_BUFFER_SIZE / sizeof(short)];

	while (read < size)
	{
		int result = sf_read_short(clip->SndFile, fileData + read, size - read);
		if (result == 0)
		{
			if (source->Loop)
			{
				sf_seek(clip->SndFile, 0, SEEK_SET);
			}
			else
			{
				break;
			}
		}
		else if (result < 0)
		{
			return false;
		}

		read += result;
	}

	if (read == 0) return false;
	alBufferData(
		buffer,
		GetFormatFromChannels(clip->Info.channels),
		fileData,
		AUDIO_STREAMING_BUFFER_SIZE,
		clip->Info.samplerate
	);

	return true;
}

void AudioSourceSetClip(audio_source *source, audio_clip *clip)
{
	source->Clip = clip;
	if (clip->Type == AudioClipType_Sound)
	{
		alSourcei(source->ID, AL_BUFFER, clip->Buffers[0]);
	}
	else
	{
		if (!Stream(source, clip->Buffers[0]) || !Stream(source, clip->Buffers[1]))
		{
			Println("error streaming");
			return;
		}
		alSourceQueueBuffers(source->ID, clip->NumBuffers, clip->Buffers);
	}
}

void AudioSourceCreate(audio_source *source, audio_clip *buffer)
{
	alGenSources(1, &source->ID);
	alSourcef(source->ID, AL_PITCH, 1);
	alSourcef(source->ID, AL_GAIN, 1);
	alSource3f(source->ID, AL_POSITION, 0, 0, 0);
	alSource3f(source->ID, AL_VELOCITY, 0, 0, 0);
	alSourcei(source->ID, AL_LOOPING, AL_FALSE);
	if (buffer != NULL)
	{
		AudioSourceSetClip(source, buffer);
	}
}

void AudioSourceSetPosition(audio_source *source, const vec3 &pos)
{
	alSource3f(source->ID, AL_POSITION, pos.x, pos.y, pos.z);
}

void AudioSourceSetPitch(audio_source *source, float pitch = 1)
{
	alSourcef(source->ID, AL_PITCH, pitch);
}

void AudioSourceSetGain(audio_source *source, float gain = 1)
{
	alSourcef(source->ID, AL_GAIN, gain);
}

void AudioSourcePlay(audio_source *source)
{
	alSourcePlay(source->ID);
}

void AudioSourcePlay(audio_source *source, audio_clip *buffer)
{
	AudioSourceSetClip(source, buffer);
	alSourcePlay(source->ID);
}

void AudioSourceStop(audio_source *source)
{
	alSourceStop(source->ID);
}

bool IsPlaying(audio_source *source)
{
	ALenum state;
	alGetSourcei(source->ID, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool AudioSourceUpdateStream(audio_source *source)
{
	if (!IsPlaying(source)) return false;

	int processed = 0;
	alGetSourcei(source->ID, AL_BUFFERS_PROCESSED, &processed);

	while (processed--)
	{
		ALuint buffer;
		alSourceUnqueueBuffers(source->ID, 1, &buffer);

		if (Stream(source, buffer))
		{
			alSourceQueueBuffers(source->ID, 1, &buffer);
		}
	}

	return true;
}

audio_clip *AudioClipCreate(allocator *alloc, audio_clip_type type)
{
	int numBuffers = 1;
	if (type == AudioClipType_Stream)
	{
		numBuffers = 2;
	}

	auto result = PushStruct<audio_clip>(alloc);
	result->Type = type;
	result->NumBuffers = numBuffers;
	result->Buffers = (ALuint *)PushSize(alloc, sizeof(ALuint)*numBuffers, "clip buffers");
	alGenBuffers(numBuffers, result->Buffers);
	return result;
}

void AudioClipSetSoundData(audio_clip *buf, short *data)
{
	alBufferData(
		buf->Buffers[0],
		GetFormatFromChannels(buf->Info.channels),
		data,
		buf->SampleCount * sizeof(short),
		buf->Info.samplerate
	);
}

void AudioListenerUpdate(camera &cam)
{
	static float *orient = new float[6];

	orient[0] = cam.View[2][0];
	orient[1] = cam.View[2][1];
	orient[2] = -cam.View[2][2];
	orient[3] = cam.View[1][0];
	orient[4] = cam.View[1][1];
	orient[5] = -cam.View[1][2];
	alListenerfv(AL_ORIENTATION, orient);
	alListenerfv(AL_POSITION, &cam.Position[0]);
	alListener3f(AL_VELOCITY, 0, 0, 0);
}

static const char *openAlErrorToString(int err)
{
	switch (err)
	{
	case AL_NO_ERROR: return "AL_NO_ERROR";
	case AL_INVALID_ENUM: return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE: return "AL_INVALID_VALUE";
	case AL_INVALID_NAME: return "AL_INVALID_NAME";
	case AL_INVALID_OPERATION: return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY: return "AL_OUT_OF_MEMORY";
	default:
		return "Unknown error code";
	}
}

static const char *openAlcErrorToString(int err)
{
	switch (err)
	{
	case ALC_NO_ERROR: return "ALC_NO_ERROR";
	case ALC_INVALID_DEVICE: return "ALC_INVALID_DEVICE";
	case ALC_INVALID_CONTEXT: return "ALC_INVALID_CONTEXT";
	default:
		return "Unknown error code";
	}
}

inline void AudioCheckError()
{
	std::cout << openAlErrorToString(alGetError()) << std::endl;
	std::cout << openAlcErrorToString(alcGetError(Global_Platform->AudioDevice)) << std::endl;
}

static void SetBpm(music_master &m, int bpm)
{
	m.Bpm = bpm;
	m.BeatTime = 60.0f / float(bpm);
}

extern "C" export_func void MusicMasterLoop(void *arg)
{
	auto m = (music_master *)arg;
	for (;;)
	{
		music_master_message msg;
		{
			std::unique_lock<std::mutex> lock(m->Mutex);
			m->ConditionVar.wait(lock, [m] { return !m->Queue.empty() || m->StopLoop; });
			if (m->StopLoop)
			{
				return;
			}

			msg = m->Queue.front();
			m->Queue.pop();
		}

		switch (msg.Type)
		{
		case MusicMasterMessageType_Tick:
		{
			if (m->StepBeat)
			{
				m->Timer += msg.Tick.DeltaTime;
				m->ElapsedTime += msg.Tick.DeltaTime;
				if (m->Timer >= m->BeatTime)
				{
					m->Timer = (m->Timer - m->BeatTime);
					m->Beat++;

					for (int i = 0; i < m->NextBeatCallbacks.Count(); i++)
					{
						auto beatMsg = m->NextBeatCallbacks.vec[i];
						if (beatMsg == NULL) continue;

						if ((m->Beat % beatMsg->Divisor) == 0)
						{
							beatMsg->Func(beatMsg->Arg);
							m->NextBeatCallbacks.Remove(i);
						}
					}

					m->NextBeatCallbacks.CheckClear();
				}
			}
			
			for (int i = 0; i < m->Sources.size(); i++)
			{
				if (m->SourcesState[i])
				{
					if (!AudioSourceUpdateStream(m->Sources[i]))
					{
						m->SourcesState[i] = false;
					}
				}
			}
			break;
		}

		case MusicMasterMessageType_PlayTrack:
		{
			int track = msg.PlayTrack.Track;
			m->SourcesState[track] = true;
			AudioSourcePlay(m->Sources[track]);
			break;
		}

		case MusicMasterMessageType_StopTrack:
		{
			int track = msg.PlayTrack.Track;
			m->SourcesState[track] = false;
			AudioSourceStop(m->Sources[track]);
			break;
		}

		case MusicMasterMessageType_OnNextBeat:
		{
			auto cb = PushStruct<next_beat_callback>(GetEntry(m->NextBeatCallbackPool));
			*cb = msg.NextBeatCallback;
			m->NextBeatCallbacks.Add(cb);
			break;
		}

		case MusicMasterMessageType_SetPitch:
		{
			float pitch = msg.SetPitch.Pitch;
			SetBpm(*m, m->Song->Bpm * pitch);

			for (auto source : m->Sources)
			{
				AudioSourceSetPitch(source, pitch);
			}
			break;
		}

		case MusicMasterMessageType_SetGain:
		{
			float gain = msg.SetGain.Gain;
			for (auto source : m->Sources)
			{
				AudioSourceSetGain(source, gain);
			}
			break;
		}
		}
	}
} 

void MusicMasterInit(game_main *g, music_master &m)
{
	m.Thread = g->Platform.CreateThread(g, "MusicMasterLoop", (void *)&m);
	m.NextBeatCallbackPool.Arena = &m.Arena;
}

void MusicMasterLoadSong(music_master &m, song *s)
{
	m.Song = s;
	m.Beat = 0;
	m.Timer = 0;
	m.ElapsedTime = 0;

	SetBpm(m, s->Bpm);
	if (s->Tracks.size() > m.Sources.size())
	{
		int previousSize = m.Sources.size();
		m.Sources.resize(s->Tracks.size());
		m.SourcesState.resize(s->Tracks.size());
		for (int i = previousSize; i < m.Sources.size(); i++)
		{
			m.Sources[i] = NULL;
		}
	}
	for (int i = 0; i < s->Tracks.size(); i++)
	{
		auto source = m.Sources[i];
		auto track = s->Tracks[i];
		assert(track->Type == AudioClipType_Stream);

		if (source == NULL)
		{
			source = PushStruct<audio_source>(m.Arena);
			AudioSourceCreate(source, track);
			m.Sources[i] = source;
			source->Loop = true;
		}
		else
		{
			AudioSourceSetClip(source, track);
		}

		m.SourcesState[i] = false;
	}
}

void MusicMasterQueueMessage(music_master &m, const music_master_message &msg)
{
	{
		std::unique_lock<std::mutex> lock(m.Mutex);
		m.Queue.push(msg);
	}
	m.ConditionVar.notify_one();
}

void MusicMasterTick(music_master &m, float dt)
{
	music_master_message msg;
	msg.Type = MusicMasterMessageType_Tick;
	msg.Tick.DeltaTime = dt;
	MusicMasterQueueMessage(m, msg);
}

void MusicMasterPlayTrack(music_master &m, int track)
{
	music_master_message msg;
	msg.Type = MusicMasterMessageType_PlayTrack;
	msg.PlayTrack.Track = track;
	MusicMasterQueueMessage(m, msg);
}

void MusicMasterStopTrack(music_master &m, int track)
{
	music_master_message msg;
	msg.Type = MusicMasterMessageType_StopTrack;
	msg.StopTrack.Track = track;
	MusicMasterQueueMessage(m, msg);
}

void MusicMasterOnNextBeat(music_master &m, next_beat_callback::func *func, void *arg, int divisor = 1)
{
	music_master_message msg;
	msg.Type = MusicMasterMessageType_OnNextBeat;
	msg.NextBeatCallback.Func = func;
	msg.NextBeatCallback.Arg = arg;
	msg.NextBeatCallback.Divisor = divisor;
	MusicMasterQueueMessage(m, msg);
}

void MusicMasterSetPitch(music_master &m, float pitch)
{
	music_master_message msg;
	msg.Type = MusicMasterMessageType_SetPitch;
	msg.SetPitch.Pitch = pitch;
	MusicMasterQueueMessage(m, msg);
}

void MusicMasterSetGain(music_master &m, float gain)
{
	music_master_message msg;
	msg.Type = MusicMasterMessageType_SetGain;
	msg.SetGain.Gain = gain;
	MusicMasterQueueMessage(m, msg);
}

void MusicMasterExit(music_master &m)
{
	m.StopLoop = true;
	m.ConditionVar.notify_one();
}