// Copyright

TWEEN_FUNC(TweenFuncLinear)
{
    return t;
}

void InitTweenState(tween_state &State)
{
    State.Funcs[TweenEasing_Linear] = TweenFuncLinear;
}

tween CreateTween(float_tween Float, float Duration, tween_easing Easing = TweenEasing_Linear)
{
    tween Result = {};
    Result.Type = TweenType_Float;
    Result.Float = Float;
    Result.Duration = Duration;
    Result.Easing = Easing;
    return Result;
}

void AddSequences(tween_state &State, tween_sequence *Seqs, int Count)
{
    for (int i = 0; i < Count; i++)
    {
        auto Seq = Seqs + i;
        State.Sequences.push_back(Seq);
        Seq->ID = State.Sequences.size() - 1;
    }
}

void DestroySequences(tween_state &State, tween_sequence *Seqs, int Count)
{
    auto Last = Seqs + (Count - 1);
    State.Sequences.erase(State.Sequences.begin() + Seqs->ID,
                          State.Sequences.begin() + Last->ID);
}

inline void AddTween(tween_sequence *Seq, const tween &t)
{
    Seq->Tweens.push_back(t);
}

void PlaySequence(tween_state &State, tween_sequence *Seq)
{
    State.ActiveSequences.push_back(Seq);
    Seq->ActiveID = State.ActiveSequences.size() - 1;
}

void StopSequence(tween_state &State, tween_sequence *Seq)
{
    auto Pos = State.ActiveSequences.begin() + Seq->ActiveID;
    State.ActiveSequences.erase(Pos);
}

void Update(tween_state &State, float Delta)
{
    for (auto Seq : State.ActiveSequences)
    {
        auto &Tween = Seq->Tweens[Seq->CurrentTween];
        switch (Tween.Type)
        {
        case TweenType_Float:
        {
            float Amount = State.Funcs[Tween.Easing](Tween.Timer);
            float Dif = Tween.Float.To - Tween.Float.From;
            *Tween.Float.Target = Tween.Float.From + Dif*Amount;
            break;
        }
        }

        if (Tween.Timer >= Tween.Duration)
        {
            Seq->CurrentTween++;
            if (Seq->CurrentTween >= Seq->Tweens.size())
            {
                Seq->CurrentTween = 0;
                if (!Seq->Loop)
                {
                    StopSequence(State, Seq);
                }
            }
        }

        Tween.Timer += Delta;
    }
}
