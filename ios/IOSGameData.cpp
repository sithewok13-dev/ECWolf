/*
** IOSGameData.cpp: where the player's own game files are, on iOS (and in
** the Linux rig, which plays the same search over a folder of its own)
**
** The files go in the app's Documents folder (Files > On My iPhone >
** Wolf3D), as they are or as the game's folder, whatever its name: GOG's
** Wolfenstein 3D folder has the *.WL6 files at its top and Spear of Destiny
** in "m1"; GOG's Spear of Destiny has "M1" and the two mission packs in "M2"
** and "M3" (their files called *.SOD, as the first mission's: wl_iwad.cpp
** names them for what they are). So the search looks in Documents and two
** levels of folders under it, and lists every folder with game files in it.
** Nothing of the game's is ever bundled.
*/

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "IOSGameData.h"

// The files ECWolf plays (by their extensions): Wolfenstein 3D (shareware,
// registered), Spear of Destiny (demo, the game, the mission packs), Super
// 3-D Noah's Ark
static const char *const gameExtensions[] = { "wl1", "wl3", "wl6", "sdm", "sod", "sd1", "sd2", "sd3", "n3d" };

static bool IsGameFile(const char *name)
{
	const char *dot = strrchr(name, '.');
	if(!dot || strncasecmp(name, "vswap.", 6) != 0) // (every game has a VSWAP)
		return false;
	for(unsigned int i = 0;i < sizeof(gameExtensions)/sizeof(gameExtensions[0]);i++)
		if(strcasecmp(dot + 1, gameExtensions[i]) == 0)
			return true;
	return false;
}

static bool IsDirectory(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool HasGameFiles(const char *dir)
{
	DIR *d = opendir(dir);
	if(!d)
		return false;
	bool found = false;
	while(struct dirent *e = readdir(d))
	{
		if(IsGameFile(e->d_name))
		{
			found = true;
			break;
		}
	}
	closedir(d);
	return found;
}

static int Search(const char *dir, int depth, char (*out)[IOSGAMEDATA_PATH], int ct, int max)
{
	if(ct < max && HasGameFiles(dir))
		snprintf(out[ct++], IOSGAMEDATA_PATH, "%s", dir);
	if(depth >= 2)
		return ct;
	DIR *d = opendir(dir);
	if(!d)
		return ct;
	// (in name order, so the same files are found first every time)
	char names[64][256];
	int n = 0;
	while(struct dirent *e = readdir(d))
	{
		if(e->d_name[0] == '.' || n >= 64)
			continue;
		snprintf(names[n++], sizeof(names[0]), "%s", e->d_name);
	}
	closedir(d);
	for(int i = 1;i < n;i++)
		for(int j = i;j > 0 && strcasecmp(names[j-1], names[j]) > 0;j--)
		{
			char t[256];
			memcpy(t, names[j], sizeof(t));
			memcpy(names[j], names[j-1], sizeof(t));
			memcpy(names[j-1], t, sizeof(t));
		}
	for(int i = 0;i < n && ct < max;i++)
	{
		char path[IOSGAMEDATA_PATH];
		snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
		if(IsDirectory(path))
			ct = Search(path, depth + 1, out, ct, max);
	}
	return ct;
}

int IOSGameData_Folders(const char *documents, char (*out)[IOSGAMEDATA_PATH], int max)
{
	if(!documents || !*documents)
		return 0;
	return Search(documents, 0, out, 0, max);
}

const char *IOSGameData_HelpTitle()
{
	return "Game files needed";
}

const char *IOSGameData_HelpText()
{
	return
		"Wolf3D plays the files from your own copy of the game (Wolfenstein 3D, "
		"Spear of Destiny and its two mission packs).\n\n"
		"Copy your game folders into this app's folder: in the Files app, On My iPhone > Wolf3D "
		"(On My iPad on an iPad). From a computer, Finder (Mac) or iTunes (Windows) can copy them "
		"in too, under Files for this app.\n\n"
		"GOG's Wolfenstein 3D: its whole folder (the *.WL6 files, and Spear of Destiny in its m1 folder). "
		"GOG's Spear of Destiny: its whole folder, with M1, M2 and M3 (the mission packs) in it. "
		"Files from other copies (Steam, CD) work too: *.WL6, *.SOD, *.SD2, *.SD3.\n\n"
		"The game starts by itself once the files are found.";
}

#ifdef ECWOLF_IOSRIG
#include <stdlib.h>
// (the rig: the folder standing in for the app's Documents)
const char *IOSGameData_Documents()
{
	const char *docs = getenv("IOSRIG_DOCUMENTS");
	return docs ? docs : "";
}
#endif
