// The CAMERA's frame grabs: see IOSCapture.h.

#include "IOSCapture.h"
#include "IOSCamera.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static int _iWanted = 0;            // the kinds the next frame should take (0: none waiting)
static int _iMode = IOSCAPTURE_MODE_PHOTO;
static long _slBytesSaving = 0;     // handed to IOSPhotos, not saved yet
static int _ctSaving = 0;
static unsigned char *_pClean = NULL;
static int _iCleanW = 0, _iCleanH = 0;
static int _iScreenW = 0, _iScreenH = 0; // the screen (FrameBegin)
static IOSCaptureStats _stats;

static double Now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Photos are the game's pixels made k times bigger (nearest), k a whole
// number taking them to 1920 wide at least (2 on a phone), 4 at most
static int Scale(void)
{
  if (_iScreenW <= 0) return 1;
  int k = (1920 + _iScreenW - 1) / _iScreenW;
  return k < 1 ? 1 : k > 4 ? 4 : k;
}

static long ImageBytes(void)
{
  const long k = Scale();
  return (_iScreenW > 0 && _iScreenH > 0) ? (long)_iScreenW * _iScreenH * 4 * k * k : 0;
}

static int NumImages(int kinds)
{
  return ((kinds & IOSCAPTURE_CLEAN) ? 1 : 0) + ((kinds & (IOSCAPTURE_HUD | IOSCAPTURE_HUDBTNS)) ? 1 : 0)
         + ((kinds & IOSCAPTURE_HUDBTNS) ? 1 : 0);
}

int IOSCapture_HasRoom(int kinds, int n)
{
  const long image = ImageBytes();
  if (image <= 0) return 0;
  return _slBytesSaving + image * NumImages(kinds & IOSCAPTURE_ALL) * n <= image * IOSCAPTURE_BUDGET_IMAGES;
}

int IOSCapture_Request(int kinds, int mode)
{
  kinds &= IOSCAPTURE_ALL;
  if (mode != IOSCAPTURE_MODE_PHOTO) kinds = IOSCAPTURE_CLEAN;
  if (!kinds || _iWanted || !IOSCapture_HasRoom(kinds, 1)) return 0;
  _iWanted = kinds;
  _iMode = mode;
  return 1;
}

int IOSCapture_IsPending(void) { return _iWanted != 0; }

void IOSCapture_Cancel(void)
{
  free(_pClean);
  _pClean = NULL;
  _iWanted = 0;
}

int IOSCapture_IsBusy(void) { return _iWanted || _ctSaving; }

void IOSCapture_Done(long bytes, double saveSeconds)
{
  if (_ctSaving > 0) _ctSaving--;
  _slBytesSaving -= bytes;
  if (_slBytesSaving < 0 || !_ctSaving) _slBytesSaving = 0;
  _stats.saveSeconds = saveSeconds;
}

void IOSCapture_TakeStats(IOSCaptureStats *pStats)
{
  if (pStats) *pStats = _stats;
  _stats.saveSeconds = 0;
  _stats.peakBytes = _slBytesSaving;
  _stats.freezeSeconds = 0;
}

void IOSCapture_NoteFreeze(double seconds) { _stats.freezeSeconds = seconds; }

// w x h 8-bit pixels made RGBA (top row first, opaque), k times bigger
static unsigned char *Convert(const unsigned char *pix, int pitch, int w, int h, const unsigned char *pal, int k)
{
  const double t0 = Now();
  const size_t outW = (size_t)w * k, row = outW * 4;
  unsigned char *p = (unsigned char *)malloc(row * h * k);
  if (!p) return NULL;
  for (int y = 0; y < h; y++) {
    const unsigned char *src = pix + (size_t)y * pitch;
    unsigned char *dst = p + row * (size_t)y * k;
    unsigned char *d = dst;
    for (int x = 0; x < w; x++) {
      const unsigned char *c = pal + src[x] * 3;
      for (int i = 0; i < k; i++, d += 4) {
        d[0] = c[0];
        d[1] = c[1];
        d[2] = c[2];
        d[3] = 255;
      }
    }
    for (int i = 1; i < k; i++) memcpy(dst + row * i, dst, row);
  }
  _stats.readSeconds = Now() - t0;
  return p;
}

static void Hand(unsigned char *pHud, int hudW, int hudH)
{
  unsigned char *pClean = _pClean;
  const int kinds = _iWanted;
  const long cleanBytes = pClean ? (long)_iCleanW * _iCleanH * 4 : 0;
  const long hudBytes = pHud ? (long)hudW * hudH * 4 * ((kinds & IOSCAPTURE_HUDBTNS) ? 2 : 1) : 0;
  _pClean = NULL;
  _iWanted = 0;
  _ctSaving++;
  _slBytesSaving += cleanBytes + hudBytes; // (what IOSPhotos hands back to IOSCapture_Done)
  if (_slBytesSaving > _stats.peakBytes) _stats.peakBytes = _slBytesSaving;
  IOSPhotos_SaveCapture(pClean, _iCleanW, _iCleanH, pHud, hudW, hudH, kinds, _iMode);
}

static void Failed(void)
{
  IOSCapture_Cancel();
  IOSCamera_Message("Couldn't take the photo");
}

void IOSCapture_FrameBegin(int screenW, int screenH)
{
  _iScreenW = screenW;
  _iScreenH = screenH;
}

void IOSCapture_AfterScene(const unsigned char *pix, int pitch, int w, int h, const unsigned char *pal)
{
  if (!(_iWanted & IOSCAPTURE_CLEAN) || _pClean || w <= 0 || h <= 0) return;
  const int k = Scale();
  _pClean = Convert(pix, pitch, w, h, pal, k);
  if (!_pClean) {
    Failed();
    return;
  }
  _iCleanW = w * k;
  _iCleanH = h * k;
  if (_iWanted == IOSCAPTURE_CLEAN) Hand(NULL, 0, 0);
}

void IOSCapture_AfterHud(const unsigned char *pix, int pitch, int w, int h, const unsigned char *pal)
{
  if (!(_iWanted & (IOSCAPTURE_HUD | IOSCAPTURE_HUDBTNS)) || w <= 0 || h <= 0) return;
  if ((_iWanted & IOSCAPTURE_CLEAN) && !_pClean) return; // (the view not drawn yet: the next frame)
  const int k = Scale();
  unsigned char *p = Convert(pix, pitch, w, h, pal, k);
  if (!p) {
    Failed();
    return;
  }
  Hand(p, w * k, h * k);
}
