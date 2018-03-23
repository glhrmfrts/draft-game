#ifndef DRAFT_MENU_STATE_H
#define DRAFT_MENU_STATE_H

enum menu_screen
{
	MenuScreen_Main,
	MenuScreen_Play,
	MenuScreen_Opts,
	MenuScreen_OptsGame,
	MenuScreen_OptsAudio,
	MenuScreen_OptsGraphics,
	MenuScreen_Cred,
	MenuScreen_Exit,
};

struct menu_state
{
	tween_sequence *FadeInSequence;
    tween_sequence *FadeOutSequence;
	menu_screen Screen = MenuScreen_Main;
    float Alpha = 1;
    int HotMainMenu = 0;
};

#endif
