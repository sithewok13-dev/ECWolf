/* The CAMERA (photos, bursts, selfies, Action Freeze, the cinematic burst),
   as the owner's Jedi Knight, Serious Sam, Dark Forces and Raze ports have
   it, for Wolfenstein 3D and Spear of Destiny in ECWolf. IOSCamera.cpp is
   the game's side, IOSCapture.cpp takes the frames, IOSPhotos.mm saves them
   on iOS (ios/rig/IOSPhotosRig.cpp on the Linux rig).

   - A photo (CAMERA tapped): the next frame, of each kind ticked in the
     options: clean (the 3D view alone: no weapon, no status bar, no
     buttons), with the status bar, with the status bar and the buttons.
     Taken at the game's own resolution and scaled up a whole number of
     times, nearest (sharp pixels).
   - A burst (CAMERA slid sideways): 10 clean photos, 0.2 s apart.
   - A selfie (CAMERA held 0.5 s): a group photo from the player's phone,
     with no player in it: the nearest enemies are brought in close and
     pose together, all facing the lens in their standing frame (now and
     then a step of their walk; never an attack, pain or death frame), an
     arrangement per photo (a huddle, faces peeking in at the frame's edge,
     rows, a corner, a tiny one far off between two faces, a totem or a
     pyramid on shoulders, one held up high); everyone else, and every
     shot, is hidden. 1, 3 or 5 photos (the options). Nobody about: "No one
     around for a group photo".
   - Action Freeze (CAMERA slid down): the game holds still and a camera
     turns round what the player was looking at: one finger orbits it, two
     move it and pinch to zoom (the lens zooms in up to 8x; pinched in past
     1x the camera backs off); it keeps out of walls and doors. BJ stands
     where the player is, drawn with the game's own BJ sprites (the end of
     an episode's victory run, from the player's Wolfenstein 3D files;
     Spear of Destiny has none, so there he isn't drawn). The shutter takes
     clean photos; X goes back.
   - The cinematic burst (CAMERA slid up): the game holds still, five
     photos as the camera sweeps round the action (the first is exactly
     what the player saw, without the weapon), then the game goes on.

   Nothing in the game changes: the photo view's camera, the poses, moves
   and hiding, BJ's figure and what the renderer marks as seen (and on the
   automap) are applied only while the view is drawn and put back exactly
   right after, every frame; the game holds still meanwhile (no tic runs),
   and the camera's random numbers are its own. Single player only. */

#ifndef WOLF_IOSCAMERA_H
#define WOLF_IOSCAMERA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* What the overlay calls (IOSTouch.m) */
void IOSCamera_Photo(void);       /* CAMERA tapped (in Action Freeze: the shutter) */
void IOSCamera_Selfie(void);      /* CAMERA held IOSTOUCH_SELFIE_HOLD */
void IOSCamera_Burst(void);       /* CAMERA slid sideways */
void IOSCamera_Freeze(void);      /* CAMERA slid down (in Action Freeze: out of it) */
void IOSCamera_Cinematic(void);   /* CAMERA slid up */
int IOSCamera_IsBusy(void);       /* a selfie, a freeze, a cinematic burst, a burst or a photo waiting: CAMERA dimmed, no new press */
int IOSCamera_Stage(void);        /* 0 none, 1 selfie, 2 Action Freeze, 3 cinematic burst: the overlay hides its buttons */
void IOSCamera_ShowOptions(void); /* the tray's CAMERA held: the options (IOSPhotos) */

/* The options (IOSPhotos keeps them in the app's settings): what a tap
   takes (IOSCAPTURE_* bits), photos per selfie (1, 3, 5), FREEZE on */
int IOSCamera_GetKinds(void);
void IOSCamera_SetKinds(int kinds);
int IOSCamera_GetSelfieShots(void);
void IOSCamera_SetSelfieShots(int n);
int IOSCamera_GetFreezeOn(void);
void IOSCamera_SetFreezeOn(int on);

/* Action Freeze's moves (IOSPhotos' view on iOS; the rig's commands):
   degrees round the focus (+yaw to the left, +pitch from higher up); the
   focus moved across the view (fractions of the view's height: right,
   down); the pinch (> 1 in) */
int IOSCamera_IsFrozen(void);
void IOSCamera_FreezeOrbit(float yawDeg, float pitchDeg);
void IOSCamera_FreezePan(float right, float down);
void IOSCamera_FreezeZoom(float factor);
float IOSCamera_FreezeZoomNow(void);
void IOSCamera_FreezeEnd(void);
int IOSCamera_NumTaken(void);     /* photos so far in this selfie or cinematic burst */

/* The host's side (IOSPhotos.mm, or the rig's) */
void IOSPhotos_Init(void);              /* once: the remembered options */
void IOSPhotos_FreezeShown(int bOn);    /* Action Freeze's controls on or off */
void IOSCamera_Message(const char *str);/* a line for the player (the overlay's message line) */

#ifdef __cplusplus
}

/* The game's side: what IOSGame.cpp and ECWolf's renderer call */
// Once a frame from IOSGame_Frame (bPlay: a level being played, single
// player, no menu): the bursts, the selfie's and cinema's photos, the freeze
// camera; anything else ends whatever the camera was doing
void IOSCamera_Frame(bool bPlay);
// The play loop: the game holds still (a selfie, Action Freeze, cinema)
bool IOSCamera_HoldsGame();
// A save, a load, a level change: everything ends now
void IOSCamera_Abort();
// ThreeDRefresh, around the 3D view: the photo view's poses, moves and
// hiding applied (and the lens's field of view), then everything put back
void IOSCamera_RenderBegin();
void IOSCamera_RenderEnd();
// CalcViewVariables: the photo view's camera instead of the player's eyes
// (viewx, viewy, viewangle); WallRefresh: its height (viewz) and tilt
// (viewshift)
bool IOSCamera_View(int32_t &viewX, int32_t &viewY, uint32_t &angle);
void IOSCamera_ViewHeight(int &viewShift, int32_t &viewZ);
// CalcRotate: the angle sprites are seen from
uint32_t IOSCamera_RotateAngle(uint32_t playerAngle);
// R_RenderView: before the weapon (the clean photo); whether it is drawn
void IOSCamera_AfterScene(unsigned char *vbuf, unsigned pitch, int w, int h);
bool IOSCamera_HidesWeapon();
// The play loop, after the frame went to the screen: the photo with the
// status bar
void IOSCamera_FrameDone();
#ifdef ECWOLF_IOSRIG
// The rig's "cam ..." commands
void IOSCamera_RigCommand(const char *cmd);
#endif
#endif

#endif /* WOLF_IOSCAMERA_H */
