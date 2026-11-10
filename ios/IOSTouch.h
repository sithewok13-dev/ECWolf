/* On-screen touch controls for iOS, for Wolfenstein 3D and Spear of Destiny
   in ECWolf: the controls standard of the owner's other ports (Jedi Knight,
   Serious Sam, Dark Forces), ported from the owner's UZDoom port's
   IOSTouch.m (itself from the Raze and Serious Sam ports').

   A UIKit overlay (IOSTouch.m) sits on top of SDL's game view. While a level
   is being played it turns touches into:
     - a floating move stick under the left thumb (anywhere in the left 42%
       of the screen): analog movement, up the screen is forward, sideways
       strafes
     - drag-to-look anywhere else: turning (Wolfenstein has no looking up
       and down)
     - tilt aiming (MENU's tray's GYRO: OFF, TOUCH, ALWAYS; SENS 1.0x, 1.5x,
       2.0x, 3.0x): turning too
     - buttons. Bottom right: FIRE, with USE (doors, switches), MAP (the
       automap) and RUN (always run on or off, lit while on: ECWolf's own
       Always Run setting) on an arc around it; dragging on them also turns.
       Top left: NEXT WPN. Top right: QUICK SAVE, QUICK LOAD (0.3 s holds
       with a ring) and MENU (a tap: the game's menu; held 0.45 s: the tray).
     - NEXT WPN: a quick tap is the next weapon; a 10 pt slide opens the
       weapon wheel at once, a 0.3 s hold opens it to tap. One slice per
       weapon in the player's weapon slots (knife, pistol, machine gun, chain
       gun), with its picture from the game's own art and its ammo. The game
       holds still while it is open.
     - MENU's tray: SENS, GYRO, OPACITY, FPS and the keyboard (the iOS
       keyboard, typing into the game), with the build stamp under it.
   In ECWolf's menus, the title and story screens, intermissions and
   messages (IOSTOUCH_MENUS) a tap is Return (select, go on), a slide up,
   down, left or right moves that way (one step for each 30 pt), BACK is
   Escape and the arrows on the right step up and down.

   Threads: ECWolf runs its game and its input on the main thread, so every
   call here is the main thread's. */

#ifndef WOLF_IOSTOUCH_H
#define WOLF_IOSTOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* What the game is doing (IOSTouch_Update), which decides what shows */
enum {
  IOSTOUCH_HIDDEN = 0, /* nothing: touches go to SDL (before the game's window, while loading) */
  IOSTOUCH_GAMEPLAY,   /* a level is being played */
  IOSTOUCH_MENUS,      /* the menus, screens waiting for a key, messages: taps and slides are keys */
  IOSTOUCH_PAUSED,     /* the game is paused (the pause key): RESUME and MENU */
};

/* What the game says each frame besides the weapons */
typedef struct IOSTouchHud {
  int bValid;
  int bNoSaves;        /* no QUICK SAVE / QUICK LOAD (a network game) */
  int bRunOn;          /* always run is on (RUN lit) */
  int bAutomap;        /* the automap is open (MAP lit) */
  int bKeyboard;       /* the iOS keyboard is up (the tray's keyboard lit) */
  float afHudFrame[4]; /* (filled by the overlay: IOSTouch_GetHudFrame) */
} IOSTouchHud;

/* What the player asked for, carried out by the game (IOSTouch_Update's result) */
enum {
  IOSTOUCH_REQ_MENU      = 1 << 0, /* Escape: the menu (in the menus: back) */
  IOSTOUCH_REQ_KEYBOARD  = 1 << 1, /* the iOS keyboard up or down (MENU's tray) */
  IOSTOUCH_REQ_QUICKSAVE = 1 << 2,
  IOSTOUCH_REQ_QUICKLOAD = 1 << 3,
  IOSTOUCH_REQ_RESUME    = 1 << 4, /* unpause */
  IOSTOUCH_REQ_MAP       = 1 << 5, /* MAP: open or close the automap */
  IOSTOUCH_REQ_RUN       = 1 << 6, /* RUN: always run on or off */
};

/* The keys the menus get (IOSTouch_TakeMenuKey), one at a time */
enum {
  IOSTOUCH_KEY_NONE = 0,
  IOSTOUCH_KEY_UP, IOSTOUCH_KEY_DOWN, IOSTOUCH_KEY_LEFT, IOSTOUCH_KEY_RIGHT,
  IOSTOUCH_KEY_ENTER, IOSTOUCH_KEY_ESCAPE,
};

/* Buttons as the game reads them (IOSTouchInput.ulButtons) */
enum {
  IOSTOUCH_FIRE       = 1 << 0,
  IOSTOUCH_USE        = 1 << 1, /* open doors, push switches */
  IOSTOUCH_NEXTWEAPON = 1 << 2,
  IOSTOUCH_NUMBUTTONS = 3
};
typedef struct IOSTouchInput {
  float fMoveX;           /* move stick: -1..1, right is + */
  float fMoveY;           /* -1..1, forward is + */
  unsigned int ulButtons; /* IOSTOUCH_FIRE... held (or a quick tap) for this read */
  int iSelectWeapon;      /* -1 none, else the wheel's pick (an index into IOSTouchWeapons' list):
                             reported for two reads, then -1 for at least one, never in a read with
                             NEXT or the read after one */
} IOSTouchInput;

/* The weapon wheel: what the player has, as the game tells it every frame
   (before IOSTouch_Update). The game makes the list (the player's weapon
   slots, in the order NEXT WPN goes through them); weapon numbers are
   indexes into it, 0..ctWeapons-1. */
#define IOSTOUCH_WPN_MAX 14
#define IOSTOUCH_WPN_NAME 32
/* The slices' colours, by the kind of ammo the weapon takes */
enum { IOSTOUCH_WCOL_NONE = 0, IOSTOUCH_WCOL_SHELLS, IOSTOUCH_WCOL_BULLETS, IOSTOUCH_WCOL_ROCKETS, IOSTOUCH_WCOL_GRENADES,
       IOSTOUCH_WCOL_NAPALM, IOSTOUCH_WCOL_SNIPER, IOSTOUCH_WCOL_ELECTRICITY, IOSTOUCH_WCOL_CANNONBALLS, IOSTOUCH_WCOL_COUNT };
typedef struct IOSTouchWeapons {
  int bValid;            /* the player and its weapons exist */
  int bCanSelect;        /* alive, in a level: it would take a pick now */
  int ctWeapons;         /* the list's length (0: no wheel; the taps still step through the weapons) */
  unsigned int uList;    /* changes whenever the list does (another game, a mod): the wheel's icons are asked again */
  char astrName[IOSTOUCH_WPN_MAX][IOSTOUCH_WPN_NAME]; /* each weapon's name */
  int aiColour[IOSTOUCH_WPN_MAX];                     /* IOSTOUCH_WCOL_* */
  int iWanted;           /* the weapon in hand, or being switched to (-1: none of the list) */
  unsigned int ulOwned;  /* bit w: weapon w owned */
  unsigned int ulReady;  /* bit w: owned and it has ammo for it -- what the game would switch to */
  int bInfiniteAmmo;     /* no counts shown */
  int aiAmmo[IOSTOUCH_WPN_MAX];     /* the ammo count for weapon w, -1: none to show */
  int aiMaxAmmo[IOSTOUCH_WPN_MAX];
} IOSTouchWeapons;

/* The pictures (IOSGame.cpp makes them from the game's own art, as RGBA,
   not premultiplied, at most IOSTOUCH_ICON_MAX square): the weapons whose
   wheel icon the overlay still wants (bit w; each asked for once per list,
   until it comes), each one handed over (NULL: there is none, the slice
   shows the name) */
#define IOSTOUCH_ICON_MAX 128
unsigned int IOSTouch_WantedIcons(void);
void IOSTouch_SetWeaponIcon(int iWeapon, const unsigned char *pubRGBA, int iWidth, int iHeight);

/* Once per frame: shows or hides the overlay for iMode, follows the SDL
   window (an SDL_Window *). ulFramesDrawn: the game's count of frames drawn,
   which the FPS readout counts. Returns the IOSTOUCH_REQ_* the player asked
   for since the last call. */
int IOSTouch_Update(void *pSDLWindow, int iMode, const IOSTouchHud *pHud, unsigned int ulFramesDrawn);
/* The next key for the menus (IOSTOUCH_KEY_*; IOSTOUCH_KEY_NONE: none) */
int IOSTouch_TakeMenuKey(void);
/* Where the HUD lays itself out: the screen less its rounded corners, the
   home indicator and a notch, as x0, y0, x1, y1 fractions of the screen. The
   whole screen until IOSTouch_Update has seen SDL's view. */
void IOSTouch_GetHudFrame(float afFrame[4]);
/* The frame rate asked of the screen: fixed at 60 in Wolf3D (no switch in
   the tray). The game keeps to it too (i_ios.mm: IOS_FrameCap) */
int IOSTouch_MaxFps(void);
/* Hide now (the app is going to the background), and stop tilt aiming's
   motion updates; the next update shows it again */
void IOSTouch_Hide(void);
/* Once per game tic: the stick and the buttons. A button tapped too quickly
   for any tic to see it is reported for two reads, then up for one. */
void IOSTouch_ReadInput(IOSTouchInput *pInput);
/* Look movement since the last call, in points of finger travel x the look
   speed (+x right, +y down) */
void IOSTouch_TakeLook(float *pfDX, float *pfDY);
/* Tilt aiming since the last call, in degrees for the view to turn (+ left)
   and tilt (+ up) */
void IOSTouch_TakeGyro(float *pfYaw, float *pfPitch);

/* Every frame before IOSTouch_Update: what the player has, for the weapon
   wheel. A wheel open while the game couldn't take a pick (or whose list
   changed) closes, picking nothing. */
void IOSTouch_SetWeapons(const IOSTouchWeapons *pw);
/* Whether the weapon wheel is open in a level being played: the game holds
   still */
int IOSTouch_HoldsGame(void);
/* A line of news over the game for fSeconds */
void IOSTouch_ShowMessage(const char *str, double fSeconds);

#ifdef __cplusplus
}
#endif

#endif /* WOLF_IOSTOUCH_H */
