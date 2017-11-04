#ifndef DRAFT_MENU_STATE_H
#define DRAFT_MENU_STATE_H

struct menu_axis
{
    uint8 Type;
    float Timer = 0.0f;
};

struct menu_state
{
    tween_sequence SubMenuChangeSequence;
    menu_axis HorizontalAxis;
    menu_axis VerticalAxis;
    int SelectedMainMenu = -1;
    int HotMainMenu = 0;
    float SubMenuChangeTimer = 0.0f;
};

#endif
