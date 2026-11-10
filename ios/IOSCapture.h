/* The CAMERA's frame grabs (as the Jedi Knight, Serious Sam and Raze ports'
   iosCapture): a request for the next frame, taken from ECWolf's 8-bit
   screen at two points of it -- once the 3D view is drawn, before the
   weapon (clean: the view window alone), and once the frame is on the
   screen (the whole screen, status bar and all) -- made RGBA with the
   game's palette, scaled up a whole number of times (nearest: sharp
   pixels) and handed to IOSPhotos to save. A budget keeps the images
   waiting to be saved within a few screens' worth. Plain C, on the game's
   thread (the main thread on iOS); the Linux rig drives this very code. */

#ifndef WOLF_IOSCAPTURE_H
#define WOLF_IOSCAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

/* What a photo is of (the PHOTO option's ticks; a bit mask) */
#define IOSCAPTURE_CLEAN    1 /* the 3D view alone */
#define IOSCAPTURE_HUD      2 /* the screen, with the status bar and the weapon */
#define IOSCAPTURE_HUDBTNS  4 /* ...and the touch buttons, as the screen shows */
#define IOSCAPTURE_ALL      7

/* Full-screen images that may wait to be saved at once */
#define IOSCAPTURE_BUDGET_IMAGES 6

/* What a capture is for (a burst's and a selfie's are clean only) */
#define IOSCAPTURE_MODE_PHOTO  0
#define IOSCAPTURE_MODE_BURST  1
#define IOSCAPTURE_MODE_SELFIE 2

/* Asks for these kinds of the next frame: 0 if one is waiting already or
   there is no room in the budget */
int IOSCapture_Request(int kinds, int mode);
int IOSCapture_HasRoom(int kinds, int n);
int IOSCapture_IsPending(void);
void IOSCapture_Cancel(void);
int IOSCapture_IsBusy(void);
/* (IOSPhotos) one capture of `bytes` saved or given up, saveSeconds after it was handed over */
void IOSCapture_Done(long bytes, double saveSeconds);

typedef struct {
  double readSeconds;
  double saveSeconds;
  long peakBytes;
  double freezeSeconds;
} IOSCaptureStats;
void IOSCapture_TakeStats(IOSCaptureStats *pStats);
void IOSCapture_NoteFreeze(double seconds);

/* The frame (8-bit pixels, pitch bytes a row, pal: 256 RGB triplets). The
   screen's size first (screenW: the scale-up follows it); the 3D view
   before the weapon; the whole screen once the frame is shown. */
void IOSCapture_FrameBegin(int screenW, int screenH);
void IOSCapture_AfterScene(const unsigned char *pix, int pitch, int w, int h, const unsigned char *pal);
void IOSCapture_AfterHud(const unsigned char *pix, int pitch, int w, int h, const unsigned char *pal);

/* IOSPhotos: takes the images (malloc'd RGBA, top row first, opaque;
   either may be NULL) and frees them. The HUD one is the whole game
   picture (for the buttons drawn over it). */
void IOSPhotos_SaveCapture(unsigned char *pClean, int cleanW, int cleanH, unsigned char *pHud, int hudW, int hudH,
                           int kinds, int mode);
/* While a burst or a selfie of several goes on: one message for them all */
void IOSPhotos_SetBatch(int bOn);

#ifdef __cplusplus
}
#endif

#endif /* WOLF_IOSCAPTURE_H */
