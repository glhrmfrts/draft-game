#ifndef DRAFT_TWEEN_H
#define DRAFT_TWEEN_H

#define TWEEN_FUNC(name) float name(float t)
typedef TWEEN_FUNC(tween_func);

enum tween_easing
{
    TweenEasing_Linear,
    TweenEasing_MAX,
};

enum tween_type
{
    TweenType_Float,
};

struct float_tween
{
    float From;
    float To;
    float *Target;
};

struct tween
{
    tween_type Type;
    float Duration = 0;
    float Timer = 0;
    tween_easing Easing = TweenEasing_Linear;

    union
    {
        float_tween Float;
    };
};

struct tween_sequence
{
    std::vector<tween> Tweens;
    int CurrentTween = 0;
    int ID = -1;
    int ActiveID = -1;
    bool Loop = false;
};

struct tween_state
{
    std::vector<tween_sequence *> Sequences;
    std::vector<tween_sequence *> ActiveSequences;

    tween_func *Funcs[TweenEasing_MAX];
};

#endif
