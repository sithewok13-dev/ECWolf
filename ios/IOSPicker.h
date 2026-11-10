// The game picker (ios/IOSPicker.mm): a card for each game ECWolf found in
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

#ifdef __cplusplus
}
#endif
