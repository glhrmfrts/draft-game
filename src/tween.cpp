// Copyright

struct sequence_id
{
    int Active;
    int Main;
};

struct sequence_data
{
    tween *Tweens;
    int TweenCount;
    int CurrentTween;
};

struct tween_id
{
    int Main;
};

enum tween_type
{
    TweenType_Float,
};

struct tween_data
{
    tween_type Type;
};

struct tween_state
{
    std::vector<sequence_data *> Sequences;
    std::vector<sequence_id> ActiveSequences;

    std::vector<tween_data *> Tweens;
};

void CreateSequences(tween_state *State, sequence_id *Seqs, int Count, int Offset = 0)
{
    for (int i = 0; i < Count; i++)
    {
        State->Sequences->push_back(PushStruct<sequence_data>(State->Arena));
        auto *Seq = Seqs + Offset + i;
        Seq->Main = State->Sequences.size() - 1;
    }
}

void CreateTweens(tween_state *State, tween_id *ts, int Count, int Offset = 0)
{
    for (int i = 0; i < Count; i++)
    {
        State->Sequences->push_back(PushStruct<sequence_data>(State->Arena));
        auto *t = ts + Offset + i;
        t->Main = State->Sequences.size() - 1;
    }
}

void BindTweens(tween_state *State, sequence_id Seq, tween_id *ts, int Count, int Offset = 0)
{
    auto *Data = State->Sequences[Seq->Main];
    Data->TweenCount = Count;
    Data->Tweens = ts + Offset;
}

void SetTweenData(tween_state *State, tween_id t, tween_type Type)
{
}

void PlaySequence(tween_state *State, sequence_id Seq)
{
	State->ActiveSequences.push_back(Seq);
	Seq->Active = State->ActiveSequences.size() - 1;
}

void PauseSequence(tween_state *State, sequence_id Seq)
{
}

void StopSequence(tween_state *State, sequence_id Seq)
{
}

void Update(tween_state *State, float Delta)
{
    for (auto &SeqId : State->ActiveSequences)
    {
        auto *Seq = State->Sequences[SeqId->Main];
        auto *TweenId = Seq->Tweens + Seq->ActiveTween;
        auto *Tween = State->Tweens[TweenId->Main];
        switch (Tween->Type)
        {
        case TweenType_Float:
            *Tween->Value = State->Funcs[Tween->Easing](*Tween->Value, Tween->From, Tween->To, Tween->Duration, Delta);
            break;
        }

        bool Done = false;
        if (Done)
        {
            Seq->ActiveTween++;
            if (Seq->ActiveTween >= Seq->TweenCount)
            {
                if (Seq->Loop)
                {
                    Seq->ActiveTween = 0;
                }
                else
                {
                    StopSequence(State, SeqId);
                }
            }
        }
    }
}
