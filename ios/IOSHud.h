/* The touch HUD, ECWolf's side (IOSHud.cpp): the pills of SPLIT and STRIP
   (IOSTouch.h's IOSTOUCH_HUD_*), and CLASSIC's status bar faded by the HUD
   slider. Built with ECWOLF_TOUCH. */
#pragma once

// R_RenderView, after the weapon (the screen locked): the pills over the
// 3D view, where IOSTouch_GetHudLayout puts them (SPLIT, STRIP), or the
// game's status bar (CLASSIC SPLIT, CLASSIC STRIP); none in the CAMERA's
// photo views. A clean photo is taken before the weapon, so it
// has none; a HUD photo has them.
void IOSHud_Draw();
// WolfStatusBar::DrawStatusBar, around its drawing: CLASSIC with the HUD
// slider under 100% fades the bar into the view's border (Begin: true if it
// does, then End after the bar is drawn)
bool IOSHud_StatusBarFadeBegin();
void IOSHud_StatusBarFadeEnd();
// CLASSIC SPLIT / STRIP: the bar's top as last drawn, a fraction of the
// screen's height (0: none), for FIRE and USE to keep above
float IOSHud_BarTop();
// (wolf_sbar.cpp) where BJ's face's box ends and the health's begins in the
// status bar's 320 columns: CLASSIC SPLIT cuts the bar between them
int IOSHud_BarFaceRight();
int IOSHud_BarHealthLeft();
