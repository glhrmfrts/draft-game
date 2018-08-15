#ifndef DRAFT_TWEEN_H
#define DRAFT_TWEEN_H

#define FAST_TWEEN_DURATION 0.5f
#define SLOW_TWEEN_DURATION 1.0f

#define TWEEN_FUNC(name) float name(float t)
typedef TWEEN_FUNC(tween_func);

struct tween
{
    std::function<void()> Callback;
    float *Value;
    float From;
    float To;
    float Duration = 0;
    float Timer = 0;
    tween_easing Easing = TweenEasing_Linear;

    tween(float *v) : Value(v) {}

    inline tween &SetFrom(float f)
    {
        From = f;
        return *this;
    }

    inline tween &SetTo(float t)
    {
        To = t;
        return *this;
    }

    inline tween &SetDuration(float d)
    {
        Duration = d;
        return *this;
    }

    inline tween &SetEasing(tween_easing e)
    {
        Easing = e;
        return *this;
    }

    inline tween &SetCallback(std::function<void()> callback)
    {
        Callback = callback;
        return *this;
    }
};

struct tween_sequence
{
    std::vector<tween> Tweens;
    int CurrentTween = 0;
    int ID = -1;
    bool Complete = false;
    bool Active = false;
    bool Loop = false;
    bool OneShot = false;
};

struct tween_state
{
    memory_arena Arena;
    generic_pool<tween_sequence> SequencePool;

    std::vector<tween_sequence *> Sequences;
    tween_func *Funcs[TweenEasing_MAX];
};

#endif
