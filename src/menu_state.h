#ifndef DRAFT_MENU_STATE_H
#define DRAFT_MENU_STATE_H

struct menu_state
{
	tween_sequence *FadeInSequence;
    tween_sequence *FadeOutSequence;
	menu_screen Screen = MenuScreen_Main;
    float Alpha = 1;
    int HotMainMenu = 0;
};

#endif
