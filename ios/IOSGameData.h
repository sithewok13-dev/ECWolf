// Where the player's own game files are (ios/IOSGameData.cpp): every
// folder with game files in it, in Documents and two levels down.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define IOSGAMEDATA_PATH 1024
#define IOSGAMEDATA_MAX 32

// The folders under documents (Documents itself too) that hold game files,
// into out (at most max); returns how many
int IOSGameData_Folders(const char *documents, char (*out)[IOSGAMEDATA_PATH], int max);
// The first screen's words, while none are found
const char *IOSGameData_HelpTitle(void);
const char *IOSGameData_HelpText(void);
// The folder searched: the app's Documents (iOS: i_ios.mm; the rig: the
// IOSRIG_DOCUMENTS folder)
const char *IOSGameData_Documents(void);

#ifdef __cplusplus
}
#endif
