#ifndef DRAFT_TWEEN_H
#define DRAFT_TWEEN_H

#define TWEEN_FUNC(name) float name(float t)
typedef TWEEN_FUNC(tween_func);

enum tween_easing
{
    TweenEasing_Linear,
    TweenEasing_MAX,
};

struct tween
{
    float *Value;
    float From;
    float To;
    float Duration = 0;
    float Timer = 0;
    tween_easing Easing = TweenEasing_Linear;
};

struct tween_sequence
{
    std::vector<tween> Tweens;
    int CurrentTween = 0;
    int ID = -1;
    bool Active = false;
    bool Loop = false;
};

struct tween_state
{
    std::vector<tween_sequence *> Sequences;
    tween_func *Funcs[TweenEasing_MAX];
};

#endif
