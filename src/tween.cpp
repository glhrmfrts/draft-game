// Copyright

TWEEN_FUNC(TweenFuncLinear)
{
    return t;
}

static void InitTweenState(tween_state &state)
{
    state.Funcs[TweenEasing_Linear] = TweenFuncLinear;
}

static void AddSequences(tween_state &state, tween_sequence *seqs, int count)
{
    for (int i = 0; i < count; i++)
    {
        auto seq = seqs + i;
        state.Sequences.push_back(seq);
        seq->ID = state.Sequences.size() - 1;
    }
}

static void DestroySequences(tween_state &state, tween_sequence *seqs, int count)
{
    for (int i = 0; i < count; i++)
    {
        auto seq = seqs + i;
        seq->Tweens.clear();
        state.Sequences[seq->ID] = NULL;
    }
}

inline static void Clear(tween_state &state)
{
    state.Sequences.clear();
}

inline static void AddTween(tween_sequence *seq, const tween &t)
{
    seq->Tweens.push_back(t);
}

static void PlaySequence(tween_state &state, tween_sequence *seq, bool reset = false)
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

static void Update(tween_state &state, float delta)
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
                seq->CurrentTween++;
                if (seq->CurrentTween >= seq->Tweens.size())
                {
                    seq->CurrentTween = 0;
                    if (!seq->Loop)
                    {
                        seq->Complete = true;
                        seq->Active = false;
                    }
                }
            }

            t.Timer += delta;
        }
    }
}

inline static tween WaitTween(float duration)
{
    return tween(NULL).SetDuration(duration);
}

inline static tween ReverseTween(const tween &t)
{
    return tween(t.Value)
        .SetFrom(t.To)
        .SetTo(t.From)
        .SetDuration(t.Duration)
        .SetEasing(t.Easing);
}
