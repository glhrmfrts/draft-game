// Copyright

TWEEN_FUNC(TweenFuncLinear)
{
    return t;
}

void InitTweenState(tween_state &state)
{
	state.SequencePool.Arena = &state.Arena;
    state.Funcs[TweenEasing_Linear] = TweenFuncLinear;
}

tween_sequence *CreateSequence(tween_state &state)
{
	auto result = PushStruct<tween_sequence>(GetEntry(state.SequencePool));
	state.Sequences.push_back(result);
	result->ID = state.Sequences.size() - 1;
	return result;
}

void DestroySequence(tween_state &state, tween_sequence *seq)
{
	state.Sequences[seq->ID] = NULL;
	FreeEntryFromData(state.SequencePool, seq);
}

inline void Clear(tween_state &state)
{
    state.Sequences.clear();
}

inline void AddTween(tween_sequence *seq, const tween &t)
{
    seq->Tweens.push_back(t);
}

void PlaySequence(tween_state &state, tween_sequence *seq, bool reset = false)
{
    if (!seq->Active)
    {
        seq->Complete = false;
        seq->Active = true;
    }
    if (reset)
    {
        auto &t = seq->Tweens[0];
        t.Timer = 0.0f;
        if (t.Value != NULL)
        {
            *t.Value = t.From;
        }
    }
}

void Update(tween_state &state, float delta)
{
    for (auto seq : state.Sequences)
    {
        if (seq && seq->Active)
        {
            auto &t = seq->Tweens[seq->CurrentTween];
            if (t.Value != NULL)
            {
                float amount = state.Funcs[t.Easing](t.Timer/t.Duration);
                float dif = t.To - t.From;
                *t.Value = t.From + dif*amount;
            }
            if (t.Timer >= t.Duration)
            {
				t.Timer = 0;
				if (t.Callback)
				{
					t.Callback();
				}
                seq->CurrentTween++;
                if (seq->CurrentTween >= seq->Tweens.size())
                {
                    seq->CurrentTween = 0;
                    if (!seq->Loop)
                    {
                        seq->Complete = true;
                        seq->Active = false;
                    }
					else if (seq->OneShot)
					{
						DestroySequence(state, seq);
					}
                }
            }

            t.Timer += delta;
        }
    }
}

inline tween WaitTween(float duration)
{
    return tween(NULL).SetDuration(duration);
}

inline tween ReverseTween(const tween &t)
{
    return tween(t.Value)
        .SetFrom(t.To)
        .SetTo(t.From)
        .SetDuration(t.Duration)
        .SetEasing(t.Easing);
}

inline tween FadeInTween(float *value, float duration)
{
    return tween(value)
        .SetFrom(0.0f)
        .SetTo(1.0f)
        .SetDuration(duration);
}

inline tween FadeOutTween(float *value, float duration)
{
    return ReverseTween(FadeInTween(value, duration));
}

inline tween CallbackTween(std::function<void()> callback)
{
    return tween(NULL).SetCallback(callback);
}