/* The touch controls' game side (IOSGame.cpp, from the owner's UZDoom
   port's ios/IOSGame.h): what ECWolf's input, play loop and status calls.
   Built with ECWOLF_TOUCH (the iOS app, and the Linux test rig). */
#pragma once

#include "IOSTouch.h"

// IN_ProcessEvents / IN_WaitAndProcessEvents, after SDL's events came in
// (so in every loop of the game's: play, menus, screens waiting for a key):
// tells the overlay what the game is doing and what the player has, and
// passes on what the player asked for (the menu, the menus' keys, the
// automap, always run, the keyboard; quick save and load wait for
// IOSGame_CheckKeys)
void IOSGame_Frame();
// PlayLoop, around its ProcessEvents: the frames being played (the overlay
// shows the game's controls only then)
void IOSGame_SetInPlay(bool bInPlay);
// PollControls, once a tic, after the keyboard, mouse and joystick: the
// stick, the buttons, the look (bFirstTic: the first tic of a frame takes
// the frame's look and tilt) and the weapon wheel's pick into the tic's
// command
void IOSGame_Ticcmd(bool bFirstTic);
// CheckKeys, in the play loop: quick save and quick load, asked for
void IOSGame_CheckKeys();
// PlayLoop: the game holds still (the weapon wheel is open; CAMERA's
// selfie, Action Freeze or cinematic burst)
bool IOSGame_HoldsGame();
// PlayLoop, after PlayFrame: the frame is on the screen (CAMERA's photos
// with the status bar)
void IOSGame_FrameDone();
// SDLFB::Update, as a frame goes to the screen: counted for the FPS
// readout; and the window the game draws in
void IOSGame_FrameDrawn();
void IOSGame_SetWindow(struct SDL_Window *pWindow);
void IOSGame_WindowGone(struct SDL_Window *pWindow);
// The games found (wl_iwad.cpp, once the one to play is known): for the
// tray's GAMES ("Switch game"), shown when there is more than one
void IOSGame_SetGames(const char *const *names, int ct, int current);
// US_LineInput (a save's name...): the iOS keyboard comes up for it
void IOSGame_TextInput(bool bOn);

// The picture on a phone's screen (sdlvideo.cpp, c_cvars.cpp): ECWolf only
// lays itself out right at 4:3, 16:10, 16:9 and 64:27 (any other shape it
// takes for 4:3 with stretched pixels), so the game draws at whichever of
// those fills most of the screen above the home indicator (64:27 on an
// iPhone with a Dynamic Island: the full width), at half the screen's
// pixels (sharp: shown at exactly 2x, nearest), placed above the home
// indicator. false: no phone screen (the desktop build, a rig run without
// IOSRIG_SCREEN)
bool IOSGame_GameResolution(int *w, int *h);
// Where the picture (texW x texH) goes in SDL's output (outW x outH pixels)
bool IOSGame_PictureRect(struct SDL_Window *pWindow, int outW, int outH, int texW, int texH, struct SDL_Rect *dst);
