#ifndef DRAFT_MENU_STATE_H
#define DRAFT_MENU_STATE_H

struct menu_state
{
    tween_sequence FadeOutSequence;
    float Alpha = 1;
    int SelectedMainMenu = -1;
    int HotMainMenu = 0;
};

#endif
