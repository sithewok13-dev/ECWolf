/*
** IOSGame.cpp: the touch controls' game side (see IOSGame.h, IOSTouch.h),
** for Wolfenstein 3D and Spear of Destiny in ECWolf; from the owner's UZDoom
** port's ios/IOSGame.cpp
**
** Once a frame (from ECWolf's event pumping, so in every one of its loops)
** it tells the overlay what the game is doing (a level being played, the
** menus and screens waiting for a key, or paused), what the player has for
** the weapon wheel (the player's weapon slots, so a mod's weapons come
** too), and passes on what the player asked for: the menu (Escape), the
** menus' keys (taps and slides, pressed as real keys, one at a time), the
** automap, always run, the iOS keyboard, SAVE's and LOAD's menus (F2, F3);
** quick save and load go in the play loop's CheckKeys. Once a tic the stick
** (running on its run marker), the buttons, the look (turning: Wolfenstein
** has no looking up and down) and the weapon picks go into the tic's
** command, whatever keys are bound; and the sithewok cheat is kept up (god
** mode, every weapon and key, ammo full, noclip; its list of levels).
*/

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include <SDL.h>

#include "wl_def.h"
#include "id_in.h"
#include "id_us.h"
#include "id_vl.h"
#include "c_cvars.h"
#include "am_map.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_main.h"
#include "wl_debug.h"	// (before wl_net.h, which names its DebugCmd)
#include "wl_net.h"
#include "wl_loadsave.h"
#include "id_sd.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "g_shared/a_keys.h"
#include "weaponslots.h"
#include "a_inventory.h"
#include "r_sprites.h"
#include "textures/textures.h"
#include "textures/bitmap.h"
#include "thingdef/thingdef.h"

#include "IOSGame.h"
#include "IOSCamera.h"
#ifdef ECWOLF_IOS
#include "IOSPicker.h"
#endif

namespace GameSave
{
	const char *TouchQuickSave();	// wl_loadsave.cpp: the slot's name, or NULL
	int TouchQuickLoad();		// 1 loaded, 0 nothing to load, -1 it failed
}
extern bool menusAreFaded;	// wl_menu.cpp: no menu up
void IOSCheat_GiveAll(player_t &player);	// wl_debug.cpp: every weapon and the ammo

static SDL_Window *gameWindow = NULL;
static bool inPlay = false;
static unsigned int framesDrawn = 0;
static int lastMode = IOSTOUCH_HIDDEN; // (what IOSGame_Frame last showed)
// The games found, the one played, "Switch game" up over the game (the
// tray's GAMES, below)
#define IOSGAME_MAX_GAMES 16
static char gameNames[IOSGAME_MAX_GAMES][96];
static int gameCount = 0, gameNow = -1;
static bool switchOpen = false;
static void SwitchGame();

void IOSGame_SetWindow(SDL_Window *pWindow)
{
	gameWindow = pWindow;
}

void IOSGame_WindowGone(SDL_Window *pWindow)
{
	if(gameWindow == pWindow)
	{
		IOSTouch_Hide();
		gameWindow = NULL;
	}
}

//---------------------------------------------------------------------------
// The picture's size and place on the screen
//---------------------------------------------------------------------------

#ifdef ECWOLF_IOS
// (i_ios.mm) the screen in points, landscape, its pixels a point, and the
// home indicator's inset at the bottom (points); the inset of SDL's view now
extern "C" void IOS_ScreenGeometry(double *ptW, double *ptH, double *scale, double *bottomPt);
extern "C" double IOS_ViewBottomInset(SDL_Window *pWindow, int outH);
#else
// The rig: IOSRIG_SCREEN="<points wide> <points high> <scale> <bottom inset>"
// stands in for a phone (run on an X screen of its pixels, --fullscreen)
static bool RigGeometry(double *ptW, double *ptH, double *scale, double *bottomPt)
{
	const char *env = getenv("IOSRIG_SCREEN");
	return env && sscanf(env, "%lf %lf %lf %lf", ptW, ptH, scale, bottomPt) == 4 && *ptW > 0 && *ptH > 0 && *scale > 0;
}
#endif

bool IOSGame_GameResolution(int *w, int *h)
{
	double ptW, ptH, scale, bottomPt;
#ifdef ECWOLF_IOS
	IOS_ScreenGeometry(&ptW, &ptH, &scale, &bottomPt);
#else
	if(!RigGeometry(&ptW, &ptH, &scale, &bottomPt))
		return false;
#endif
	const double W = ptW * scale, H = (ptH - bottomPt) * scale;
	// the shapes ECWolf knows (wl_main.cpp: CheckRatio), the one shown biggest
	static const double shapes[][2] = { { 4, 3 }, { 16, 10 }, { 16, 9 }, { 64, 27 } };
	double best = 0, bestArea = 0;
	for(unsigned int i = 0;i < 4;i++)
	{
		const double a = shapes[i][0] / shapes[i][1];
		const double pw = W < H * a ? W : H * a;
		if(pw * (pw / a) > bestArea * 1.01)
		{
			bestArea = pw * (pw / a);
			best = a;
		}
	}
	double ph = (W < H * best ? W : H * best) / best;
	// half the screen's pixels once that's still 360 rows or more (the
	// game draws in software: a quarter of the work)
	const int div = ph >= 720 ? 2 : 1;
	int gh = ((int)(ph / div)) & ~1;
	int gw = ((int)(gh * best + 0.5)) & ~1;
	while(gw * div > (int)W && gh > 200)
	{
		gh -= 2;
		gw = ((int)(gh * best + 0.5)) & ~1;
	}
	if(gw < 320 || gh < 200)
		return false;
	*w = gw;
	*h = gh;
	return true;
}

static int lastPicture[6] = { 0, 0, 0, 0, 0, 0 };

bool IOSGame_PictureRect(SDL_Window *pWindow, int outW, int outH, int texW, int texH, SDL_Rect *dst)
{
	if(outW <= 0 || outH <= 0 || texW <= 0 || texH <= 0)
		return false;
	double bottom = 0;
#ifdef ECWOLF_IOS
	bottom = IOS_ViewBottomInset(pWindow, outH);
#else
	(void)pWindow;
	double ptW, ptH, scale, bottomPt;
	if(!RigGeometry(&ptW, &ptH, &scale, &bottomPt))
		return false;
	bottom = bottomPt * scale;
#endif
	if(bottom < 0 || bottom > outH / 4)
		bottom = 0;
	const double availH = outH - bottom;
	double k = (double)outW / texW;
	if(availH / texH < k)
		k = availH / texH;
	// a whole number of the screen's pixels to the game's, when that loses
	// little (sharp, even pixels)
	if(k >= 1 && floor(k) / k >= 0.9)
		k = floor(k);
	const int w = (int)(texW * k + 0.5), h = (int)(texH * k + 0.5);
	int y = (outH - h) / 2;
	if(y + h > (int)availH)
		y = (int)availH - h;
	if(y < 0)
		y = 0;
	dst->x = (outW - w) / 2;
	dst->y = y;
	dst->w = w;
	dst->h = h;
	lastPicture[0] = dst->x;
	lastPicture[1] = dst->y;
	lastPicture[2] = w;
	lastPicture[3] = h;
	lastPicture[4] = outW;
	lastPicture[5] = outH;
	return true;
}

// The status bar's top on the screen (a fraction of the screen's height
// from its top; 0: no status bar): FIRE and USE keep above it
static float StatusTop()
{
	if(viewsize >= 21 || screenHeight <= 0 || statusbary2 <= 0 || (unsigned)statusbary2 >= screenHeight || lastPicture[5] <= 0)
		return 0;
	return (float)((lastPicture[1] + (double)statusbary2 * lastPicture[3] / screenHeight) / lastPicture[5]);
}

// (CAMERA's photo with the buttons: where the game's picture was last put,
// in SDL's output pixels: x, y, w, h, and the output's width and height)
extern "C" int IOSGame_LastPictureRect(int r[6])
{
	if(lastPicture[4] <= 0)
		return 0;
	memcpy(r, lastPicture, sizeof(lastPicture));
	return 1;
}

// For the log: every 5 s of play, the frames a second the game really drew
// and the time between them (average and longest)
static void LogFrameRate()
{
	static Uint64 start = 0, last = 0, longest = 0;
	static unsigned int frames = 0;
	const Uint64 now = SDL_GetPerformanceCounter(), freq = SDL_GetPerformanceFrequency();
	// (the mode the last events were pumped in: in play, the frame is too)
	if(lastMode != IOSTOUCH_GAMEPLAY || (last && now - last > freq / 4))
	{
		start = last = 0; // (not in play, or a pause or a load: start again)
		if(lastMode != IOSTOUCH_GAMEPLAY)
			return;
	}
	if(!start)
	{
		start = last = now;
		frames = 0;
		longest = 0;
		return;
	}
	frames++;
	if(now - last > longest)
		longest = now - last;
	last = now;
	if(now - start >= freq * 5)
	{
		const double secs = (double)(now - start) / freq;
		printf("iOS: %.1f fps in play, %.1f ms a frame (longest %.1f ms), %dx%d\n", frames / secs, secs * 1000.0 / frames,
			longest * 1000.0 / freq, screenWidth, screenHeight);
		fflush(stdout);
		start = last = now;
		frames = 0;
		longest = 0;
	}
}

void IOSGame_FrameDrawn()
{
	framesDrawn++;
	LogFrameRate();
}

void IOSGame_SetInPlay(bool bInPlay)
{
	inPlay = bInPlay;
}

static player_t *Player()
{
	player_t *p = &players[ConsolePlayer];
	return p->mo != NULL ? p : NULL;
}

// What the overlay shows: the game's controls in the frames the play loop
// pumps (no menu up, the level going on, no demo), RESUME while paused, and
// the menus' taps and slides everywhere else (the menus, the title and
// story screens, intermissions, messages waiting for a key)
static int TouchMode()
{
	if(!gameWindow)
		return IOSTOUCH_HIDDEN;
	if(inPlay && menusAreFaded && playstate == ex_stillplaying && !demoplayback && Player())
		return (Paused & 1) ? IOSTOUCH_PAUSED : IOSTOUCH_GAMEPLAY;
	return IOSTOUCH_MENUS;
}

//---------------------------------------------------------------------------
//
// Keys, pressed as the keyboard's: one at a time, held IOSTOUCH_KEY_MS (so
// a loop that reads the keys held sees it), then a gap
//
//---------------------------------------------------------------------------

#define IOSGAME_KEY_MS 60
#define IOSGAME_KEY_GAP_MS 30

static SDL_Scancode keyDown = SDL_SCANCODE_UNKNOWN;
static Uint32 keyUpAt = 0, keyNextAt = 0;
static bool escapeWanted = false;
static SDL_Scancode menuKeyWanted = SDL_SCANCODE_UNKNOWN; // SAVE's or LOAD's tap: F2 or F3

static void PostKey(SDL_Scancode sc, bool down)
{
	SDL_Event ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
	ev.key.windowID = gameWindow ? SDL_GetWindowID(gameWindow) : 0;
	ev.key.keysym.scancode = sc;
	ev.key.keysym.sym = SDL_GetKeyFromScancode(sc);
	SDL_PushEvent(&ev);
}

static SDL_Scancode ScancodeOf(int key)
{
	switch(key)
	{
		case IOSTOUCH_KEY_UP: return SDL_SCANCODE_UP;
		case IOSTOUCH_KEY_DOWN: return SDL_SCANCODE_DOWN;
		case IOSTOUCH_KEY_LEFT: return SDL_SCANCODE_LEFT;
		case IOSTOUCH_KEY_RIGHT: return SDL_SCANCODE_RIGHT;
		case IOSTOUCH_KEY_ENTER: return SDL_SCANCODE_RETURN;
		case IOSTOUCH_KEY_ESCAPE: return SDL_SCANCODE_ESCAPE;
		default: return SDL_SCANCODE_UNKNOWN;
	}
}

static void PumpKeys()
{
	const Uint32 now = SDL_GetTicks();
	if(keyDown != SDL_SCANCODE_UNKNOWN)
	{
		if((Sint32)(now - keyUpAt) < 0)
			return;
		PostKey(keyDown, false);
		keyDown = SDL_SCANCODE_UNKNOWN;
		keyNextAt = now + IOSGAME_KEY_GAP_MS;
		return;
	}
	if((Sint32)(now - keyNextAt) < 0)
		return;
	SDL_Scancode sc = SDL_SCANCODE_UNKNOWN;
	if(escapeWanted)
	{
		escapeWanted = false;
		sc = SDL_SCANCODE_ESCAPE;
	}
	else if(menuKeyWanted != SDL_SCANCODE_UNKNOWN)
	{
		sc = menuKeyWanted;
		menuKeyWanted = SDL_SCANCODE_UNKNOWN;
	}
	else
		sc = ScancodeOf(IOSTouch_TakeMenuKey());
	if(sc == SDL_SCANCODE_UNKNOWN)
		return;
	PostKey(sc, true);
	keyDown = sc;
	keyUpAt = now + IOSGAME_KEY_MS;
}

//---------------------------------------------------------------------------
//
// The weapon wheel: the player's weapon slots in order (Wolfenstein: knife,
// pistol, machine gun, chain gun), each with its ammo and the picture of it
// in hand (its Ready state's sprite)
//
//---------------------------------------------------------------------------

static const ClassDef *wheelClasses[IOSTOUCH_WPN_MAX];
static int wheelSlot[IOSTOUCH_WPN_MAX], wheelSlotSize[IOSTOUCH_WPN_MAX];
static int wheelCount = 0;
static unsigned int wheelList = 0;

static void BuildWheelList(player_t *p)
{
	wheelCount = 0;
	unsigned int h = 2166136261u;
	for(int slot = 0;slot < NUM_WEAPON_SLOTS && wheelCount < IOSTOUCH_WPN_MAX;slot++)
	{
		// (slot 0 after 9, as the keys are)
		const int s = (slot + 1) % NUM_WEAPON_SLOTS;
		const FWeaponSlot &ws = p->weapons.Slots[s];
		for(int i = 0;i < ws.Size() && wheelCount < IOSTOUCH_WPN_MAX;i++)
		{
			const ClassDef *cls = ws.GetWeapon(i);
			if(!cls)
				continue;
			bool dup = false;
			for(int k = 0;k < wheelCount;k++)
				if(wheelClasses[k] == cls)
					dup = true;
			if(dup)
				continue;
			wheelSlot[wheelCount] = s;
			wheelSlotSize[wheelCount] = ws.Size();
			wheelClasses[wheelCount++] = cls;
			h = (h ^ (unsigned int)(uintptr_t)cls) * 16777619u;
		}
	}
	wheelList = (h ^ (unsigned int)wheelCount) | 1u; // (never 0)
}

// The weapon's name on its slice: Wolfenstein's own names, else the class
// name with spaces ("MacFlameThrower": "MAC FLAME THROWER")
static void WeaponName(const ClassDef *cls, char *out, size_t size)
{
	static const struct { const char *cls, *name; } known[] = {
		{ "Knife", "KNIFE" }, { "Pistol", "PISTOL" }, { "MachineGun", "MACHINE GUN" },
		{ "GatlingGun", "CHAIN GUN" }, { "MacFlameThrower", "FLAME THROWER" },
		{ "MacRocketLauncher", "ROCKET LAUNCHER" },
	};
	const char *n = cls->GetName().GetChars();
	for(unsigned int i = 0;i < countof(known);i++)
	{
		if(stricmp(n, known[i].cls) == 0)
		{
			snprintf(out, size, "%s", known[i].name);
			return;
		}
	}
	size_t o = 0;
	for(size_t i = 0;n[i] && o + 2 < size;i++)
	{
		if(i > 0 && isupper((unsigned char)n[i]) && islower((unsigned char)n[i-1]))
			out[o++] = ' ';
		out[o++] = (char)toupper((unsigned char)n[i]);
	}
	out[o] = 0;
}

static int WheelIndexOf(const AWeapon *weap)
{
	if(!weap || weap == WP_NOCHANGE)
		return -1;
	for(int i = 0;i < wheelCount;i++)
		if(weap->GetClass() == wheelClasses[i])
			return i;
	return -1;
}

static bool CanSelect(const player_t *p)
{
	return p && p->state == player_t::PST_LIVE && p->health > 0 && !(p->flags & player_t::PF_DISABLESWITCH)
		&& playstate == ex_stillplaying;
}

static void FillWeapons(IOSTouchWeapons *pw)
{
	player_t *p = Player();
	if(!p)
		return;
	BuildWheelList(p);
	pw->bValid = 1;
	pw->bCanSelect = CanSelect(p);
	pw->ctWeapons = wheelCount;
	pw->uList = wheelList;
	pw->bInfiniteAmmo = ammocheat;
	const AWeapon *wanted = (p->PendingWeapon && p->PendingWeapon != WP_NOCHANGE) ? p->PendingWeapon : p->ReadyWeapon;
	pw->iWanted = WheelIndexOf(wanted);
	for(int i = 0;i < wheelCount;i++)
	{
		const ClassDef *cls = wheelClasses[i];
		WeaponName(cls, pw->astrName[i], IOSTOUCH_WPN_NAME);
		const AWeapon *def = static_cast<const AWeapon *>(cls->GetDefault());
		const ClassDef *ammoType = def ? def->ammotype[0] : NULL;
		pw->aiColour[i] = ammoType ? IOSTOUCH_WCOL_BULLETS : IOSTOUCH_WCOL_NONE;
		pw->aiAmmo[i] = pw->aiMaxAmmo[i] = -1;
		AWeapon *weap = static_cast<AWeapon *>(p->mo->FindInventory(cls));
		if(ammoType)
		{
			AInventory *ammo = p->mo->FindInventory(ammoType);
			const AInventory *ammoDef = static_cast<const AInventory *>(ammoType->GetDefault());
			pw->aiAmmo[i] = ammo ? (int)ammo->amount : 0;
			pw->aiMaxAmmo[i] = ammo ? (int)ammo->maxamount : (ammoDef ? (int)ammoDef->maxamount : 0);
		}
		if(weap)
		{
			pw->ulOwned |= 1u << i;
			if(weap->CheckAmmo(AWeapon::EitherFire, false))
				pw->ulReady |= 1u << i;
		}
	}
}

// The wheel's pick: its slot's key where it is the slot's only weapon (as
// the player would press it), else the weapon itself
static int pickSlot = -1;
static void PickWeapon(int i)
{
	player_t *p = Player();
	if(!CanSelect(p) || i < 0 || i >= wheelCount)
		return;
	if(wheelSlotSize[i] == 1)
	{
		pickSlot = wheelSlot[i];
		return;
	}
	AWeapon *weap = static_cast<AWeapon *>(p->mo->FindInventory(wheelClasses[i]));
	if(weap && weap != p->ReadyWeapon)
		p->PendingWeapon = weap;
}

// The picture of a weapon in hand, from the game's own art: as RGBA, cropped
// to what isn't see-through and scaled to fit IOSTOUCH_ICON_MAX
static bool MakeIcon(FTexture *tex, TArray<BYTE> &out, int &ow, int &oh)
{
	if(!tex || tex->GetWidth() <= 0 || tex->GetHeight() <= 0)
		return false;
	FBitmap bmp;
	if(!bmp.Create(tex->GetWidth(), tex->GetHeight()))
		return false;
	memset(bmp.GetPixels(), 0, (size_t)bmp.GetPitch() * tex->GetHeight());
	tex->CopyTrueColorPixels(&bmp, 0, 0);
	const int w = bmp.GetWidth(), h = bmp.GetHeight(), pitch = bmp.GetPitch();
	const BYTE *px = bmp.GetPixels();
	int x0 = w, y0 = h, x1 = -1, y1 = -1;
	for(int y = 0;y < h;y++)
		for(int x = 0;x < w;x++)
			if(px[y*pitch + x*4 + 3] > 8)
			{
				if(x < x0) x0 = x;
				if(x > x1) x1 = x;
				if(y < y0) y0 = y;
				if(y > y1) y1 = y;
			}
	if(x1 < x0 || y1 < y0)
		return false;
	const int cw = x1 - x0 + 1, ch = y1 - y0 + 1, big = MAX(cw, ch);
	// (whole steps up, so the pixels stay sharp; down by averaging)
	const int k = MAX(1, IOSTOUCH_ICON_MAX / big);
	if(big * k <= IOSTOUCH_ICON_MAX && big <= IOSTOUCH_ICON_MAX)
	{
		ow = cw * k;
		oh = ch * k;
		out.Resize(ow * oh * 4);
		for(int y = 0;y < oh;y++)
			for(int x = 0;x < ow;x++)
			{
				const BYTE *s = px + (y0 + y / k) * pitch + (x0 + x / k) * 4;
				BYTE *d = &out[(y * ow + x) * 4];
				d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = s[3]; // (BGRA to RGBA)
			}
		return true;
	}
	ow = MAX(1, cw * IOSTOUCH_ICON_MAX / big);
	oh = MAX(1, ch * IOSTOUCH_ICON_MAX / big);
	out.Resize(ow * oh * 4);
	for(int y = 0;y < oh;y++)
		for(int x = 0;x < ow;x++)
		{
			const int sx0 = x0 + x * cw / ow, sx1 = x0 + MAX((x + 1) * cw / ow, x * cw / ow + 1);
			const int sy0 = y0 + y * ch / oh, sy1 = y0 + MAX((y + 1) * ch / oh, y * ch / oh + 1);
			double r = 0, g = 0, b = 0, a = 0;
			int n = 0;
			for(int sy = sy0;sy < sy1;sy++)
				for(int sx = sx0;sx < sx1;sx++, n++)
				{
					const BYTE *s = px + sy * pitch + sx * 4;
					const double al = s[3];
					r += s[2] * al; g += s[1] * al; b += s[0] * al; a += al;
				}
			BYTE *d = &out[(y * ow + x) * 4];
			if(a > 0)
			{
				d[0] = (BYTE)(r / a + 0.5); d[1] = (BYTE)(g / a + 0.5); d[2] = (BYTE)(b / a + 0.5);
			}
			else
				d[0] = d[1] = d[2] = 0;
			d[3] = (BYTE)(a / MAX(n, 1) + 0.5);
		}
	return true;
}

static FTexture *WeaponTexture(const ClassDef *cls)
{
	const AWeapon *def = static_cast<const AWeapon *>(cls->GetDefault());
	if(!def)
		return NULL;
	FTexture *tex = R_GetFrameTexture(def->GetReadyState());
	if(!tex)
		tex = R_GetFrameTexture(def->SpawnState);
	return tex;
}

static void FeedIcons()
{
	const unsigned int wanted = IOSTouch_WantedIcons();
	if(!wanted)
		return;
	TArray<BYTE> px;
	int w = 0, h = 0;
	for(int i = 0;i < IOSTOUCH_WPN_MAX;i++)
	{
		if(!(wanted & (1u << i)))
			continue;
		if(i < wheelCount && MakeIcon(WeaponTexture(wheelClasses[i]), px, w, h))
			IOSTouch_SetWeaponIcon(i, &px[0], w, h);
		else
			IOSTouch_SetWeaponIcon(i, NULL, 0, 0); // (its name shows)
	}
}

//---------------------------------------------------------------------------
//
// The sithewok cheat (IOSTouch.h): while it's on, god mode, every weapon and
// key (given as it comes on and in every level started or loaded), the ammo
// kept full, noclip while USE's hold has it on. Turned off, what it gave is
// kept; god mode and noclip go off (if the cheat put them on). Single
// player only. Its list of levels: the current game's maps (MAP01...), by
// episode and floor.
//
//---------------------------------------------------------------------------

static bool cheatGod = false, cheatNoclip = false;
static const AActor *cheatGivenMo = NULL;
static char cheatGivenMap[9] = "";
static int32_t cheatLastTime = 0;
// Every level started or loaded: the player's body, the map or the clock
// going back changed (for IOSTouchHud.uWorld, and the cheat's giving)
static const AActor *worldMo = NULL;
static char worldMap[9] = "";
static int32_t worldTime = 0;
static unsigned int worldCount = 1;

static bool CheatAllowed()
{
	return Net::InitVars.mode == Net::MODE_SinglePlayer;
}

static void NoteWorld(const player_t *p)
{
	if(!p)
		return;
	if(p->mo != worldMo || strncmp(worldMap, gamestate.mapname, 8) != 0 || (int32_t)gamestate.TimeCount < worldTime)
	{
		worldMo = p->mo;
		IOSCamera_Abort(); // (CAMERA: whatever it was doing ends with the level)
		strncpy(worldMap, gamestate.mapname, 8);
		worldMap[8] = 0;
		worldCount++;
	}
	worldTime = gamestate.TimeCount;
}

// Once a tic in play
static void KeepCheat()
{
	player_t *p = Player();
	const bool on = p && CheatAllowed() && IOSTouch_CheatOn();
	if(!on)
	{
		if(cheatGod)
			godmode = 0;
		if(cheatNoclip)
			noclip = false;
		cheatGod = cheatNoclip = false;
		cheatGivenMo = NULL;
		return;
	}
	if(!godmode)
	{
		godmode = 1;
		cheatGod = true;
	}
	const bool wantNoclip = IOSTouch_CheatNoclip() != 0;
	if(wantNoclip != noclip && (wantNoclip || cheatNoclip))
	{
		noclip = wantNoclip;
		cheatNoclip = wantNoclip;
	}
	// everything, as it comes on and in each level started or loaded
	if(p->mo != cheatGivenMo || strncmp(cheatGivenMap, gamestate.mapname, 8) != 0 || (int32_t)gamestate.TimeCount < cheatLastTime)
	{
		cheatGivenMo = p->mo;
		strncpy(cheatGivenMap, gamestate.mapname, 8);
		cheatGivenMap[8] = 0;
		IOSCheat_GiveAll(*p);
		P_GiveKeys(p->mo, 101);
		if(p->health < 100)
			p->mo->health = p->health = 100;
		if(StatusBar)
			StatusBar->DrawStatusBar();
		printf("iOS: cheat gave everything (%s)\n", gamestate.mapname);
	}
	cheatLastTime = gamestate.TimeCount;
	// the ammo kept full
	for(AInventory *item = p->mo->inventory;item;item = item->inventory)
	{
		if(item->IsKindOf(NATIVE_CLASS(Ammo)) && item->amount < item->maxamount)
			item->amount = item->maxamount;
	}
}

// The levels: the maps this game has, in order
#define IOSGAME_MAX_LEVELS 99
static char levelNames[IOSGAME_MAX_LEVELS][9];
static int levelCount = -1;

static void FindLevels()
{
	if(levelCount >= 0)
		return;
	levelCount = 0;
	for(int i = 1;i <= IOSGAME_MAX_LEVELS;i++)
	{
		FString name;
		name.Format("MAP%02d", i);
		if(GameMap::CheckMapExists(name))
		{
			strncpy(levelNames[levelCount], name.GetChars(), 8);
			levelNames[levelCount++][8] = 0;
		}
	}
}

static int CurrentLevel()
{
	for(int i = 0;i < levelCount;i++)
		if(stricmp(levelNames[i], gamestate.mapname) == 0)
			return i + 1;
	return 0;
}

// An episode's name, its last line ("Episode 1\nEscape from Wolfenstein")
static FString EpisodeTitle(unsigned int e)
{
	FString n = EpisodeInfo::GetEpisode(e).EpisodeName;
	const long nl = n.LastIndexOf('\n');
	if(nl >= 0)
		n = n.Mid(nl + 1);
	return n;
}

int IOSGame_LevelInfo(int i, char *strName, int ctName, char *strDetail, int ctDetail)
{
	FindLevels();
	if(i < 1 || i > levelCount)
		return 0;
	const char *map = levelNames[i - 1];
	const LevelInfo &info = LevelInfo::Find(map);
	const bool known = stricmp(info.MapName, map) == 0;
	// its episode: the one whose first map is in its cluster
	int episode = -1;
	for(unsigned int e = 0;known && e < EpisodeInfo::GetNumEpisodes();e++)
	{
		const LevelInfo &start = LevelInfo::Find(EpisodeInfo::GetEpisode(e).StartMap);
		if(stricmp(start.MapName, EpisodeInfo::GetEpisode(e).StartMap) == 0 && start.Cluster == info.Cluster)
		{
			episode = (int)e;
			break;
		}
	}
	// a secret floor: some map's secret exit goes there
	bool secret = false;
	for(int k = 0;k < levelCount && known;k++)
	{
		const LevelInfo &o = LevelInfo::Find(levelNames[k]);
		if(k != i - 1 && stricmp(o.MapName, levelNames[k]) == 0 && o.NextSecret.CompareNoCase(map) == 0 && o.NextSecret.CompareNoCase(o.NextMap) != 0)
			secret = true;
	}
	// "Floor 3", or its MAPINFO name (Spear of Destiny's "Tunnels 1"...)
	FString name, detail;
	const FString *own = known ? info.GetMapInfoName() : NULL;
	if(own && own->Len() > 0)
		name.Format("%s%s", own->GetChars(), secret ? " (secret)" : "");
	else if(known && info.FloorNumber.Len() > 0)
		name.Format("Floor %s%s", info.FloorNumber.GetChars(), secret ? " (secret)" : "");
	else
		name = map;
	if(episode >= 0 && EpisodeInfo::GetNumEpisodes() > 1)
	{
		const FString title = EpisodeTitle(episode);
		name = title.Len() > 0 ? FString(title + ", " + name) : name;
		detail.Format("Episode %d \xC2\xB7 %s", episode + 1, map);
	}
	else if(own && own->Len() > 0 && known && info.FloorNumber.Len() > 0)
		detail.Format("Floor %s \xC2\xB7 %s", info.FloorNumber.GetChars(), map);
	else
		detail = map;
	snprintf(strName, ctName, "%s", name.GetChars());
	snprintf(strDetail, ctDetail, "%s", detail.GetChars());
	return 1;
}

static void WarpTo(int i)
{
	FindLevels();
	player_t *p = Player();
	if(i < 1 || i > levelCount || !p || !CheatAllowed() || playstate != ex_stillplaying)
		return;
	char name[128], detail[128];
	IOSGame_LevelInfo(i, name, sizeof(name), detail, sizeof(detail));
	FString msg;
	// (ECWolf's warp: the floor from its start, as a new game -- the pistol,
	// 8 bullets, full health, score 0, the lives a new game has; with the
	// cheat on, KeepCheat gives everything again)
	if(IOSTouch_CheatOn())
		msg.Format("Going to %s", name);
	else
		msg.Format("Going to %s: a fresh start", name);
	IOSTouch_ShowMessage(msg, 2.0);
	printf("iOS: level list: to %s (%s)\n", levelNames[i - 1], name);
	DebugCmd cmd = {DEBUG_Warp};
	cmd.ArgS = levelNames[i - 1];
	Net::DebugKey(cmd);
}

// CHEAT's picture: BJ's face from the status bar, grinning -- god mode's
// (Spear of Destiny), else the grin of a chain gun found, else the plain one
static void FeedCheatIcon()
{
	if(!IOSTouch_WantsCheatIcon())
		return;
	static const char *const faces[] = { "STFGOD0", "STFEVL0", "STFST00" };
	TArray<BYTE> px;
	int w = 0, h = 0;
	for(unsigned int i = 0;i < countof(faces);i++)
	{
		const FTextureID id = TexMan.CheckForTexture(faces[i], FTexture::TEX_Any);
		if(id.isValid() && MakeIcon(TexMan[id], px, w, h))
		{
			IOSTouch_SetCheatIcon(&px[0], w, h);
			printf("iOS: CHEAT's picture is %s (%dx%d)\n", faces[i], w, h);
			return;
		}
	}
	IOSTouch_SetCheatIcon(NULL, 0, 0);
}

// The word typed: "sithewok", as letter keys going down (the iOS keyboard
// sends each letter typed as its key too, and as text: the keys alone count)
static char typed[16] = "";
static void Typed(char c)
{
	c = (char)tolower((unsigned char)c);
	if(c < 'a' || c > 'z')
		return; // (Shift goes down round a capital)
	size_t n = strlen(typed);
	if(n >= sizeof(typed) - 1)
	{
		memmove(typed, typed + 1, n);
		n--;
	}
	typed[n] = c;
	typed[n + 1] = 0;
	static const char word[] = "sithewok";
	const size_t wl = sizeof(word) - 1;
	if(n + 1 >= wl && strcmp(typed + n + 1 - wl, word) == 0)
	{
		typed[0] = 0;
		IOSTouch_CheatTyped();
	}
}

static int SDLCALL WatchTyping(void *, SDL_Event *ev)
{
	if(ev->type == SDL_KEYDOWN && !ev->key.repeat)
	{
		const SDL_Keycode k = ev->key.keysym.sym;
		if(k >= SDLK_a && k <= SDLK_z)
			Typed((char)k);
	}
	return 1;
}

//---------------------------------------------------------------------------
//
// Once a frame
//
//---------------------------------------------------------------------------

static bool wantQuickSave = false, wantQuickLoad = false;
static bool wantMap = false, wantPause = false;

#ifdef ECWOLF_IOSRIG
static void RigFrame();
#endif

void IOSGame_Frame()
{
	static bool inFrame = false;
	if(inFrame)
		return; // (a load or save it set off pumps events too)
	inFrame = true;
	static bool watching = false;
	if(!watching)
	{
		watching = true;
		SDL_AddEventWatch(WatchTyping, NULL);
	}
	const int mode = TouchMode();
	lastMode = mode;
	player_t *p = Player();
	NoteWorld(p);

	IOSTouchWeapons wpn;
	IOSTouchHud hud;
	memset(&wpn, 0, sizeof(wpn));
	memset(&hud, 0, sizeof(hud));
	if(mode == IOSTOUCH_GAMEPLAY && p)
		FillWeapons(&wpn);
	hud.bValid = 1;
	hud.bNoSaves = Net::InitVars.mode != Net::MODE_SinglePlayer;
	hud.bRunOn = alwaysrun;
	hud.bAutomap = automap != AMA_Off;
	hud.bKeyboard = SDL_IsTextInputActive();
	hud.bCheatAllowed = CheatAllowed();
	if(mode == IOSTOUCH_GAMEPLAY)
	{
		FindLevels();
		hud.ctLevels = levelCount;
		hud.iLevelNow = CurrentLevel();
	}
	hud.uWorld = worldCount;
	hud.bGames = gameCount > 1;
	hud.fStatusTop = StatusTop();
	IOSTouch_SetWeapons(&wpn);
	if(mode == IOSTOUCH_GAMEPLAY)
		FeedIcons();
	if(mode != IOSTOUCH_HIDDEN)
		FeedCheatIcon();

	const int req = IOSTouch_Update(gameWindow, mode, &hud, framesDrawn);
	// CAMERA: its bursts, selfies, Action Freeze and cinematic bursts, in a
	// level being played alone
	IOSCamera_Frame(mode == IOSTOUCH_GAMEPLAY && Net::InitVars.mode == Net::MODE_SinglePlayer);

	if(req & IOSTOUCH_REQ_MENU)
		escapeWanted = true;
	if(req & IOSTOUCH_REQ_GAMES)
		SwitchGame();
	if(req & IOSTOUCH_REQ_KEYBOARD)
		IOSGame_TextInput(!SDL_IsTextInputActive());
	if(mode == IOSTOUCH_GAMEPLAY)
	{
		if(req & IOSTOUCH_REQ_QUICKSAVE) wantQuickSave = true;
		else if(req & IOSTOUCH_REQ_QUICKLOAD) wantQuickLoad = true;
		if(req & IOSTOUCH_REQ_MAP) wantMap = true;
		if(req & IOSTOUCH_REQ_SAVEMENU) menuKeyWanted = SDL_SCANCODE_F2;
		else if(req & IOSTOUCH_REQ_LOADMENU) menuKeyWanted = SDL_SCANCODE_F3;
		if(req & IOSTOUCH_REQ_LEVEL) WarpTo(IOSTouch_TakeLevelPick());
	}
	if((req & IOSTOUCH_REQ_RESUME) && (Paused & 1))
		wantPause = true;
	if(req & IOSTOUCH_REQ_RUN)
	{
		// ECWolf's own Always Run (its Control menu's), kept in its settings
		alwaysrun = !alwaysrun;
		WriteConfig();
		IOSTouch_ShowMessage(alwaysrun ? "Always run on: the stick always runs" : "Always run off: slide up onto ^^ to run", 2.0);
	}
	if(req & IOSTOUCH_REQ_CHEATSOUND)
		SD_PlaySound("misc/yeah"); // ("Yeah!", the end of a level's 100% bonus)
	PumpKeys();
#ifdef ECWOLF_IOSRIG
	RigFrame();
#endif
	inFrame = false;
}

void IOSGame_CheckKeys()
{
	if(!wantQuickSave && !wantQuickLoad)
		return;
	const bool save = wantQuickSave;
	wantQuickSave = wantQuickLoad = false;
	IOSCamera_Abort(); // (a save or load: CAMERA's poses and views end first)
	if(Net::InitVars.mode != Net::MODE_SinglePlayer || playstate != ex_stillplaying)
		return;
	if(save)
	{
		player_t *p = Player();
		if(!p || p->state == player_t::PST_DEAD)
		{
			IOSTouch_ShowMessage("Can't save now", 2.0);
			return;
		}
		const char *name = GameSave::TouchQuickSave();
		FString msg;
		msg.Format("Quick save: %s", name ? name : "it failed");
		IOSTouch_ShowMessage(msg, 2.0);
		printf("iOS: quick save (%s)\n", name ? name : "failed");
	}
	else
	{
		const int r = GameSave::TouchQuickLoad();
		IOSTouch_ShowMessage(r > 0 ? "Quick load" : (r == 0 ? "No quick save yet (hold SAVE)" : "The quick save couldn't be loaded"), r == 0 ? 3.0 : 1.5);
		printf("iOS: quick load (%d)\n", r);
		if(r > 0)
			DrawPlayBorderSides();
	}
	IN_ClearKeysDown();
}

//---------------------------------------------------------------------------
//
// Once a tic
//
//---------------------------------------------------------------------------

static unsigned int lastButtons = 0;
static int lastPick = -1;
static double turnLeft = 0.0;
static float lastMoveX = 0.f, lastMoveY = 0.f;
static bool lastRun = false;

void IOSGame_Ticcmd(bool bFirstTic)
{
	TicCmd_t &cmd = control[ConsolePlayer];
	IOSTouchInput in;
	IOSTouch_ReadInput(&in);
	if(IOSCamera_HoldsGame())
	{
		// (CAMERA holds the game still: the touches do nothing meanwhile)
		float dx, dy;
		IOSTouch_TakeLook(&dx, &dy);
		turnLeft = 0.0;
		lastButtons = 0;
		return;
	}
	lastMoveX = in.fMoveX;
	lastMoveY = in.fMoveY;
	lastRun = in.bRun != 0;
	// (the stick on its run marker runs; with Always Run on, it always does)
	const int delta = (alwaysrun || in.bRun) ? RUNMOVE : BASEMOVE;
	KeepCheat();
	cmd.controly -= (int)lround(in.fMoveY * delta);
	cmd.controlstrafe += (int)lround(in.fMoveX * delta);
	const unsigned int b = in.ulButtons;
	if(b & IOSTOUCH_FIRE) cmd.buttonstate[bt_attack] = 1;
	if(b & IOSTOUCH_USE) cmd.buttonstate[bt_use] = 1;
	if(b & IOSTOUCH_NEXTWEAPON) cmd.buttonstate[bt_nextweapon] = 1;
	lastButtons = b;
	if(in.iSelectWeapon >= 0 && in.iSelectWeapon != lastPick)
		PickWeapon(in.iSelectWeapon);
	lastPick = in.iSelectWeapon;
	if(pickSlot >= 0)
	{
		cmd.buttonstate[bt_slot0 + pickSlot] = 1;
		pickSlot = -1;
	}
	if(wantMap)
	{
		cmd.buttonstate[bt_automap] = 1;
		wantMap = false;
	}
	if(wantPause)
	{
		cmd.buttonstate[bt_pause] = 1;
		wantPause = false;
	}
	if(bFirstTic)
	{
		// The look of the frame: turning, in the units ControlMovement
		// takes (1/ANGLESCALE = 1/20 of a degree a count; + turns right)
		float dx, dy;
		IOSTouch_TakeLook(&dx, &dy);
		if(lastMode == IOSTOUCH_GAMEPLAY && !Paused && !IOSGame_HoldsGame())
		{
			turnLeft += dx * 20.0;
			const int turn = (int)turnLeft;
			turnLeft -= turn;
			cmd.controlx += turn;
		}
		else
			turnLeft = 0.0;
	}
}

bool IOSGame_HoldsGame()
{
	return IOSTouch_HoldsGame() != 0 || IOSCamera_HoldsGame() || switchOpen;
}

//---------------------------------------------------------------------------
//
// The tray's GAMES: "Switch game" (ios/IOSPicker.mm), the games found
//
//---------------------------------------------------------------------------

void IOSGame_SetGames(const char *const *names, int ct, int current)
{
	gameCount = 0;
	for(int i = 0;i < ct && i < IOSGAME_MAX_GAMES;i++)
	{
		snprintf(gameNames[i], sizeof(gameNames[i]), "%s", names[i] ? names[i] : "?");
		gameCount++;
	}
	gameNow = current;
	printf("iOS: %d games found, playing %d (%s)\n", gameCount, gameNow, gameNow >= 0 && gameNow < gameCount ? gameNames[gameNow] : "-");
}

#ifdef ECWOLF_IOS
static void SwitchSave()
{
	WriteConfig(); // (the settings, ECWolf's own, before the app closes)
}

static void SwitchDone()
{
	switchOpen = false;
}
#endif

static void SwitchGame()
{
	if(gameCount < 2 || switchOpen)
		return;
#ifdef ECWOLF_IOS
	const char *names[IOSGAME_MAX_GAMES];
	for(int i = 0;i < gameCount;i++)
		names[i] = gameNames[i];
	switchOpen = true;
	IOSPicker_PresentSwitch(names, gameCount, gameNow, SwitchSave, SwitchDone);
#else
	printf("rig: Switch game: %d games, playing %d (%s)\n", gameCount, gameNow, gameNow >= 0 ? gameNames[gameNow] : "-");
#endif
}

void IOSGame_FrameDone()
{
	IOSCamera_FrameDone();
}

void IOSGame_TextInput(bool bOn)
{
	if(bOn && !SDL_IsTextInputActive())
		SDL_StartTextInput();
	else if(!bOn && SDL_IsTextInputActive())
		SDL_StopTextInput();
}

#ifdef ECWOLF_IOSRIG
//---------------------------------------------------------------------------
//
// The Linux rig (ios/rig): a script of touches and checks
// (IOSRIG_SCRIPT), one command a line, run a frame at a time: "frame <n>"
// waits until frame n of the game's (frames pumped), "wait <n>" n more
// frames, "waitplay" until a level is being played; "pos" says where the player is and what the game is doing,
// "shot <file>" saves the X screen as a picture,
// "key <name>" presses a key (SDL's name), "quit" ends the game; anything
// else is a touch command for the overlay (rig/IOSTouchRig.c)
//
//---------------------------------------------------------------------------

extern "C" void IOSTouchRig_Command(const char *cmd);

static void RigPos()
{
	player_t *p = Player();
	printf("rig: pos ms %u tics %d frame %u mode %d inplay %d faded %d playstate %d paused %d held %d map %s",
		(unsigned)SDL_GetTicks(), (int)gamestate.TimeCount, framesDrawn, TouchMode(), (int)inPlay, (int)menusAreFaded, (int)playstate, (int)Paused, (int)IOSTouch_HoldsGame(),
		gamestate.mapname);
	if(p)
	{
		AWeapon *w = p->ReadyWeapon;
		const AInventory *ammo = w ? (AInventory *)w->ammo[0] : NULL;
		printf(" x %.2f y %.2f angle %.1f health %d weapon %s ammo %d alwaysrun %d automap %d",
			p->mo->x / 65536.0, p->mo->y / 65536.0, p->mo->angle * (360.0 / 4294967296.0), (int)p->health,
			w ? w->GetClass()->GetName().GetChars() : "-", ammo ? (int)ammo->amount : -1, (int)alwaysrun, (int)automap);
		IOSTouchWeapons wpn;
		memset(&wpn, 0, sizeof(wpn));
		FillWeapons(&wpn);
		printf(" wheel %d owned 0x%x ready 0x%x wanted %d", wpn.ctWeapons, wpn.ulOwned, wpn.ulReady, wpn.iWanted);
		for(int i = 0;i < wpn.ctWeapons;i++)
			printf(" [%s %d icon %s]", wpn.astrName[i], wpn.aiAmmo[i], WeaponTexture(wheelClasses[i]) ? "yes" : "no");
	}
	printf(" god %d noclip %d", godmode, (int)noclip);
	if(p)
	{
		printf(" keys");
		for(AInventory *item = p->mo->inventory;item;item = item->inventory)
			if(item->IsKindOf(NATIVE_CLASS(Key)))
				printf(" %s", item->GetClass()->GetName().GetChars());
	}
	printf(" move %.2f,%.2f run %d cmd %d,%d,%d\n", lastMoveX, lastMoveY, (int)lastRun, control[ConsolePlayer].controlx, control[ConsolePlayer].controly,
		control[ConsolePlayer].controlstrafe);
	fflush(stdout);
}

static void RigFrame()
{
	static FILE *script = NULL;
	static bool opened = false;
	static unsigned int waitUntil = 0, frames = 0;
	static bool waitPlay = false;
	frames++;
	if(waitPlay)
	{
		if(TouchMode() != IOSTOUCH_GAMEPLAY)
			return;
		waitPlay = false;
	}
	if(!opened)
	{
		opened = true;
		const char *path = getenv("IOSRIG_SCRIPT");
		if(path && *path)
			script = fopen(path, "r");
		printf("rig: script %s\n", script ? path : "(none)");
	}
	if(!script || frames < waitUntil)
		return;
	char line[256];
	while(fgets(line, sizeof(line), script))
	{
		line[strcspn(line, "\r\n")] = 0;
		if(!line[0] || line[0] == '#')
			continue;
		printf("rig: > %s (frame %u)\n", line, frames);
		if(!strncmp(line, "frame ", 6))
		{
			waitUntil = (unsigned int)atoi(line + 6);
			if(frames < waitUntil)
				return;
		}
		else if(!strncmp(line, "wait ", 5))
		{
			waitUntil = frames + (unsigned int)atoi(line + 5);
			return;
		}
		else if(!strcmp(line, "waitplay"))
		{
			waitPlay = true; // (until a level is being played again)
			return;
		}
		else if(!strcmp(line, "pos"))
			RigPos();
		else if(!strncmp(line, "key ", 4))
		{
			const SDL_Scancode sc = SDL_GetScancodeFromName(line + 4);
			PostKey(sc, true);
			PostKey(sc, false);
		}
		else if(!strncmp(line, "shot ", 5))
		{
			// (the X screen the rig runs on, as a picture)
			char cmdline[512];
			snprintf(cmdline, sizeof(cmdline), "import -window root '%s' 2>/dev/null", line + 5);
			if(system(cmdline) != 0)
				printf("rig: shot failed\n");
		}
		else if(!strncmp(line, "cam ", 4))
			IOSCamera_RigCommand(line + 4); // (CAMERA: photo, selfie, freeze, orbit... see IOSCamera.cpp)
		else if(!strcmp(line, "quit"))
		{
			fflush(stdout);
			exit(0);
		}
		else
			IOSTouchRig_Command(line);
	}
	fclose(script);
	script = NULL;
	printf("rig: script done\n");
	fflush(stdout);
}
#endif
