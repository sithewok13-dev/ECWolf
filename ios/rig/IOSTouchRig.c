/* The Linux test rig for the iOS touch controls (ECWOLF_TOUCH_RIG_SOURCE):
   the overlay's plain C core (ios/IOSTouch.m, built here as C with
   IOSTOUCH_RIG, without UIKit) with a stand-in for its UIKit view: a phone's
   landscape screen (844 x 390 points, the iPhone 14's insets), the overlay's
   once-a-frame work (IOSTouch_Update) and touches from a script
   (IOSRIG_SCRIPT, read by ios/IOSGame.cpp: one command a line, after
   "frame <n>" lines saying when), so the very code the app runs is driven
   through the real game. What it does and sees goes to stdout, each line
   starting "rig:". From the owner's UZDoom port's rig.

   Touch commands (x, y in points; a place can be "@NAME", a button's
   middle, e.g. @FIRE, @NEXTWPN, @TRAYFPS, or "@NAME+dx,dy"; id: any small
   number naming the finger):
     down <id> <place>          a finger comes down
     move <id> <place>          it moves there (in 4 pt steps, as UIKit's
                                coalesced touches)
     moveby <id> <dx> <dy>      ...by this much
     up <id> | cancel <id>      it lifts (iOS takes it away)
     tapslice <i>               a tap on the open wheel's slice i
     state                      what shows, the wheel, the tray, the stick, the cheat
     levels / level <i>         the cheat's list of levels / a tap on row i
*/

#define _GNU_SOURCE
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>

#include "IOSTouch.m"

#define RIG_W 844.0
#define RIG_H 390.0
#define RIG_IN_LEFT 47.0
#define RIG_IN_TOP 0.0
#define RIG_IN_RIGHT 47.0
#define RIG_IN_BOTTOM 21.0

static double IOSTouchRig_Now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int _bRigLaidOut = 0;
static unsigned int _ulRigFrames = 0;

static void IOSTouchRig_Layout(void)
{
  const IOSTouchRect none = { 0, 0, 0, 0, 0 };
  IOSTouch_LayoutButtons(RIG_W, RIG_H, RIG_IN_LEFT, RIG_IN_TOP, RIG_IN_RIGHT, RIG_IN_BOTTOM, 1, &none, &none, &none);
  IOSTouch_SetHudFrame(RIG_W, RIG_H, RIG_IN_LEFT, RIG_IN_TOP, RIG_IN_RIGHT, RIG_IN_BOTTOM);
  _bRigLaidOut = 1;
}

static const char *const _astrModes[] = { "HIDDEN", "GAMEPLAY", "MENUS", "PAUSED" };

// As the view's IOSTouch_SetMode: every change starts clean
static void IOSTouchRig_SetMode(int iMode)
{
  if (iMode != _iMode) {
    printf("rig: mode %s -> %s\n", _astrModes[_iMode], _astrModes[iMode]);
    fflush(stdout);
  }
  _iMode = iMode;
  IOSTouch_ResetTouches(iMode == IOSTOUCH_GAMEPLAY);
}

// As the view's IOSTouch_UpdateOverlay, once a frame
int IOSTouch_Update(void *pSDLWindow, int iMode, const IOSTouchHud *pHud, unsigned int ulFramesDrawn)
{
  (void)pSDLWindow;
  const int iRequests = _iRequests;
  _iRequests = 0;
  _ulRigFrames = ulFramesDrawn;
  if (pHud) _hud = *pHud;
  if (!_bRigLaidOut || IOSTouch_StatusTopMoved()) {
    IOSTouchRig_Layout();
    printf("rig: layout: status bar top %.1f, FIRE %.1f,%.1f r %.0f, USE %.1f,%.1f r %.0f\n", _hud.fStatusTop * RIG_H,
           _aButtons[BTN_FIRE].x, _aButtons[BTN_FIRE].y, _aButtons[BTN_FIRE].radius, _aButtons[BTN_USE].x, _aButtons[BTN_USE].y,
           _aButtons[BTN_USE].radius);
    fflush(stdout);
  }
  if (iMode != _iMode) IOSTouchRig_SetMode(iMode);
  if (iMode != IOSTOUCH_HIDDEN) {
    const double now = CACurrentMediaTime();
    IOSTouch_TickHolds(now);
    IOSTouch_CountFrames(ulFramesDrawn, now);
  }
  if (iRequests) {
    printf("rig: requests 0x%x\n", iRequests);
    fflush(stdout);
  }
  return iRequests;
}

void IOSTouch_Hide(void)
{
  if (_iMode != IOSTOUCH_HIDDEN) IOSTouchRig_SetMode(IOSTOUCH_HIDDEN);
}

// ---------------------------------------------------------------- the commands

static const struct { const char *name; int button; } _aRigNames[] = {
  { "FIRE", BTN_FIRE }, { "USE", BTN_USE }, { "MAP", BTN_MAP }, { "CHEAT", BTN_CHEAT },
  { "NEXTWPN", BTN_NEXTWPN }, { "SAVE", BTN_QUICKSAVE }, { "LOAD", BTN_QUICKLOAD },
  { "MENU", BTN_MENU }, { "RESUME", BTN_RESUME }, { "BACK", BTN_BACK }, { "MENUUP", BTN_MENUUP },
  { "MENUDOWN", BTN_MENUDOWN }, { "TRAYLOOK", BTN_TRAYLOOK }, { "TRAYRUN", BTN_TRAYRUN },
  { "TRAYOPACITY", BTN_TRAYOPACITY }, { "TRAYFPS", BTN_TRAYFPS }, { "TRAYCHEAT", BTN_TRAYCHEAT },
  { "TRAYLEVEL", BTN_TRAYLEVEL }, { "CAMERA", BTN_CAMERA }, { "TRAYCAM", BTN_TRAYCAM }, { "TRAYGAMES", BTN_TRAYGAMES }, { "TRAYKEYS", BTN_TRAYKEYS },
};

// The fingers: each a touch's identity (only its address is used)
static char _acRigFingers[IOSTOUCH_MAX_TOUCHES];
static double _afRigX[IOSTOUCH_MAX_TOUCHES], _afRigY[IOSTOUCH_MAX_TOUCHES];

static const void *IOSTouchRig_Finger(int id)
{
  if (id < 0 || id >= IOSTOUCH_MAX_TOUCHES) return NULL;
  return &_acRigFingers[id];
}

// "x y", "@NAME" or "@NAME+dx,dy"; returns how many words it took (0: none)
static int IOSTouchRig_Place(char **argv, int argc, double *px, double *py)
{
  if (argc < 1) return 0;
  if (argv[0][0] == '@') {
    char name[32];
    double dx = 0.0, dy = 0.0;
    const char *plus = strpbrk(argv[0] + 1, "+-");
    const size_t n = plus ? (size_t)(plus - argv[0] - 1) : strlen(argv[0] + 1);
    snprintf(name, sizeof(name), "%.*s", (int)n, argv[0] + 1);
    if (plus) sscanf(plus, "%lf,%lf", &dx, &dy);
    for (size_t i = 0; i < sizeof(_aRigNames) / sizeof(_aRigNames[0]); i++) {
      if (!strcasecmp(name, _aRigNames[i].name)) {
        *px = _aButtons[_aRigNames[i].button].x + dx;
        *py = _aButtons[_aRigNames[i].button].y + dy;
        return 1;
      }
    }
    printf("rig: no button %s\n", name);
    return 0;
  }
  if (argc < 2) return 0;
  *px = atof(argv[0]);
  *py = atof(argv[1]);
  return 2;
}

static void IOSTouchRig_MoveTo(int id, double x, double y)
{
  const void *t = IOSTouchRig_Finger(id);
  const double x0 = _afRigX[id], y0 = _afRigY[id];
  const int steps = (int)ceil(hypot(x - x0, y - y0) / 4.0);
  for (int k = 1; k <= steps; k++) {
    const double f = (double)k / steps;
    const double cx = x0 + (x - x0) * f, cy = y0 + (y - y0) * f;
    IOSTouch_TouchPassed(t, cx, cy);
    IOSTouch_TouchMoved(t, cx, cy);
  }
  _afRigX[id] = x;
  _afRigY[id] = y;
}

static const char *IOSTouchRig_ButtonName(int b)
{
  for (size_t i = 0; i < sizeof(_aRigNames) / sizeof(_aRigNames[0]); i++) {
    if (_aRigNames[i].button == b) return _aRigNames[i].name;
  }
  return "?";
}

static void IOSTouchRig_State(void)
{
  printf("rig: state mode %s reading %d tray %d opacity %.2f/%d wheel %d tapmode %d hot %d holds %d fps %d/%d maxfps %d look %.2fx menukeys %d\n",
         _astrModes[_iMode], _bReading, _bTrayOpen, (double)_fOpacity, _bOpacityOpen, _bWheelOpen, _bWheelTapMode,
         IOSTouch_WheelHot(), IOSTouch_HoldsGame(), _bShowFps, _iFpsShown, _iMaxFps, (double)_fLookScale, _ctMenuKeys);
  printf("rig: stick %d at %.0f,%.0f move %.2f,%.2f run %d marker %d at %.0f,%.0f (deg %.0f) alwaysrun %d\n", _bStickActive,
         _fStickOriginX, _fStickOriginY, (double)_fMoveX, (double)_fMoveY, _bMoveRun, _bRunMarker, _fRunX, _fRunY, _fRunDeg,
         _hud.bRunOn);
  printf("rig: cheat shown %d on %d noclip %d allowed %d rings %.2f/%.2f levels %d now %d list %d icon %dx%d\n", _bShowCheat,
         _bCheatOn, _bNoclip, _hud.bCheatAllowed, (double)_fCheatRing, (double)_fNoclipRing, _hud.ctLevels, _hud.iLevelNow,
         _bLevelListOpen, _iCheatIconW, _iCheatIconH);
  printf("rig: shown");
  for (int i = 0; i < IOSTOUCH_NUM_BUTTONS; i++) {
    if (IOSTouch_IsShown(i)) printf(" %s(%.0f,%.0f)", IOSTouchRig_ButtonName(i), _aButtons[i].x, _aButtons[i].y);
  }
  printf("\n");
  if (_wpn.bValid) {
    printf("rig: weapons %d wanted %d owned 0x%x ready 0x%x select %d\n", _wpn.ctWeapons, _wpn.iWanted, _wpn.ulOwned, _wpn.ulReady, _wpn.bCanSelect);
  }
  if (_bWheelOpen) {
    const IOSTouchWheelMap *m = IOSTouch_WheelMapOf(_iWheelMap);
    for (int i = 0; i < m->ctSlots; i++) {
      static const char *const astrState[] = { "ready", "noammo", "notfound" };
      IOSTouchWheelLook look;
      IOSTouch_WheelSlotLook(i, IOSTouch_WheelHot(), &look);
      printf("rig: slice %d \"%s\" deg %.1f %s count %d icon %d%s%s\n", i, m->aSlots[i].strName, IOSTouch_WheelSlotDeg(m, i),
             astrState[look.iState], look.iCount, _aIcons[i].bHave, look.bWanted ? " in-hand" : "", look.bHot ? " hot" : "");
    }
    IOSTouchWheelMiddle mid;
    IOSTouch_WheelMiddleState(&mid);
    printf("rig: middle \"%s\" \"%s\" \"%s\"\n", mid.strTitle ? mid.strTitle : "", mid.strCount, mid.strHint ? mid.strHint : "");
  }
  if (_strCamMessage[0] && CACurrentMediaTime() - _tCamMessage < _fCamMessageTime) printf("rig: message \"%s\"\n", _strCamMessage);
  fflush(stdout);
}

void IOSTouchRig_Command(const char *cmd)
{
  char buf[256];
  char *argv[16];
  int argc = 0;
  snprintf(buf, sizeof(buf), "%s", cmd ? cmd : "");
  for (char *p = strtok(buf, " \t"); p && argc < 16; p = strtok(NULL, " \t")) argv[argc++] = p;
  if (argc == 0) return;
  if (!_bRigLaidOut) IOSTouchRig_Layout();
  const double now = CACurrentMediaTime();
  const char *op = argv[0];
  const int id = argc >= 2 ? atoi(argv[1]) : 0;
  double x = 0.0, y = 0.0;
  if (!strcmp(op, "down") && IOSTouchRig_Finger(id) && IOSTouchRig_Place(argv + 2, argc - 2, &x, &y)) {
    _afRigX[id] = x;
    _afRigY[id] = y;
    IOSTouch_TouchBegan(IOSTouchRig_Finger(id), x, y, RIG_W, now, now);
    printf("rig: down %d at %.0f,%.0f\n", id, x, y);
  }
  else if (!strcmp(op, "move") && IOSTouchRig_Finger(id) && IOSTouchRig_Place(argv + 2, argc - 2, &x, &y)) {
    IOSTouchRig_MoveTo(id, x, y);
  }
  else if (!strcmp(op, "moveby") && IOSTouchRig_Finger(id) && argc >= 4) {
    IOSTouchRig_MoveTo(id, _afRigX[id] + atof(argv[2]), _afRigY[id] + atof(argv[3]));
  }
  else if ((!strcmp(op, "up") || !strcmp(op, "cancel")) && IOSTouchRig_Finger(id)) {
    IOSTouch_TouchEnded(IOSTouchRig_Finger(id), _afRigX[id], _afRigY[id], !strcmp(op, "cancel"), now);
    printf("rig: %s %d\n", op, id);
  }
  else if (!strcmp(op, "tapslice") && argc >= 2) {
    // the slice's middle, half way out the ring
    const IOSTouchWheelMap *m = IOSTouch_WheelMapOf(_iWheelMap);
    const int i = atoi(argv[1]);
    if (!_bWheelOpen || i < 0 || i >= m->ctSlots) {
      printf("rig: tapslice %d: no such slice (wheel %d)\n", i, _bWheelOpen);
    }
    else {
      const double a = IOSTouch_WheelSlotDeg(m, i) * M_PI / 180.0, r = (_fWheelR0 + _fWheelR1) * 0.5;
      const void *t = IOSTouchRig_Finger(IOSTOUCH_MAX_TOUCHES - 1);
      x = _fWheelCX + r * cos(a);
      y = _fWheelCY - r * sin(a);
      IOSTouch_TouchBegan(t, x, y, RIG_W, now, now);
      IOSTouch_TouchEnded(t, x, y, 0, now);
      printf("rig: tapslice %d \"%s\" at %.0f,%.0f, wheel now %d\n", i, m->aSlots[i].strName, x, y, _bWheelOpen);
    }
  }
  else if (!strcmp(op, "level") && argc >= 2) {
    // a tap on row i (1..) of the cheat's open list of levels, as the view's
    char strName[128] = "", strDetail[128] = "";
    const int i = atoi(argv[1]);
    const int bInfo = IOSGame_LevelInfo(i, strName, sizeof(strName), strDetail, sizeof(strDetail));
    printf("rig: level %d \"%s\" \"%s\" (list %d, info %d)\n", i, strName, strDetail, _bLevelListOpen, bInfo);
    IOSTouch_PickLevel(i);
  }
  else if (!strcmp(op, "levels")) {
    // the cheat's list, as the view would show it
    for (int i = 1; i <= _hud.ctLevels; i++) {
      char strName[128] = "", strDetail[128] = "";
      if (!IOSGame_LevelInfo(i, strName, sizeof(strName), strDetail, sizeof(strDetail))) break;
      printf("rig: levels %2d%s %s | %s\n", i, i == _hud.iLevelNow ? "*" : " ", strName, strDetail);
    }
  }
  else if (!strcmp(op, "state")) {
    IOSTouchRig_State();
  }
  else {
    printf("rig: what? %s\n", cmd);
  }
  fflush(stdout);
}
