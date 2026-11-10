// The game picker (ios/IOSPicker.mm), and "Switch game" from the tray's
// GAMES: a card for each game ECWolf found in
// Files > On My iPhone > Wolf3D (Wolfenstein 3D, Spear of Destiny, Return
// to Danger, Ultimate Challenge, and any other it plays). (In the style of
// the owner's UZDoom and Raze ports' pickers.)
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define IOSPICKER_UNAVAILABLE (-2) // nothing to show (i_ios.mm shows a plain list)

// Until a game is picked. names: the games ECWolf found (its names, e.g.
// "Mission 2: Return to Danger"), iLast: the one played last. Returns the
// game's index, or IOSPICKER_UNAVAILABLE.
int IOSPicker_Run(const char *const *names, int ctNames, int iLast);

// The game picked in "Switch game" before the app closed, if that was in
// the last 10 minutes and it is among names: its index (the start goes
// straight to it); -1 otherwise. Clears it, and its notification.
int IOSPicker_TakeNext(const char *const *names, int ctNames);
// "Switch game" (the tray's GAMES), as the owner's Half-Life and Raze
// ports have it: the cards over the game, the one being played (iNow)
// marked, with Cancel. Cancel, or the game being played: pfnDone. Another:
// it is kept (IOSPicker_TakeNext), pfnSave writes the settings, and the app
// closes with a notification "Tap to start <game>" (pfnDone isn't called).
void IOSPicker_PresentSwitch(const char *const *names, int ctNames, int iNow, void (*pfnSave)(void), void (*pfnDone)(void));

#ifdef __cplusplus
}
#endif
