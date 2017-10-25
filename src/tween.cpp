// Copyright

TWEEN_FUNC(TweenFuncLinear)
{
    return t;
}

void InitTweenState(tween_state &state)
{
    state.Funcs[TweenEasing_Linear] = TweenFuncLinear;
}

void AddSequences(tween_state &state, tween_sequence *seqs, int count)
{
    for (int i = 0; i < count; i++)
    {
        auto seq = seqs + i;
        state.Sequences.push_back(seq);
        seq->ID = state.Sequences.size() - 1;
    }
}

void DestroySequences(tween_state &state, tween_sequence *seqs, int count)
{
    for (int i = 0; i < count; i++)
    {
        auto seq = seqs + i;
        state.Sequences[seq->ID] = NULL;
    }
}

inline void Clear(tween_state &state)
{
    state.Sequences.clear();
}

inline void AddTween(tween_sequence *seq, const tween &t)
{
    seq->Tweens.push_back(t);
}

void PlaySequence(tween_state &state, tween_sequence *seq)
{
    if (!seq->Active)
    {
        seq->Active = true;
        for (auto &t : seq->Tweens)
        {
            t.Timer = 0.0f;
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
            float amount = state.Funcs[t.Easing](t.Timer/t.Duration);
            float dif = t.To - t.From;
            *t.Value = t.From + dif*amount;
            if (t.Timer >= t.Duration)
            {
                seq->CurrentTween++;
                if (seq->CurrentTween >= seq->Tweens.size())
                {
                    seq->CurrentTween = 0;
                    if (!seq->Loop)
                    {
                        seq->Active = false;
                    }
                }
            }

            t.Timer += delta;
        }
    }
}
