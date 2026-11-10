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
// PlayLoop: the game holds still (the weapon wheel is open)
bool IOSGame_HoldsGame();
// SDLFB::Update, as a frame goes to the screen: counted for the FPS
// readout; and the window the game draws in
void IOSGame_FrameDrawn();
void IOSGame_SetWindow(struct SDL_Window *pWindow);
void IOSGame_WindowGone(struct SDL_Window *pWindow);
// US_LineInput (a save's name...): the iOS keyboard comes up for it
void IOSGame_TextInput(bool bOn);
