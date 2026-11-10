/*
** IOSCamera.cpp: the CAMERA's game side for Wolfenstein 3D and Spear of
** Destiny (see IOSCamera.h): photos, bursts, selfies (group photos), Action
** Freeze and the cinematic burst, after the owner's Dark Forces port's
** iosCamera.cpp and Raze port's ioscamera.cpp.
**
** The photo view: while a selfie, Action Freeze or the cinematic burst is
** on, the game holds still (no tic runs) and the 3D view is drawn from a
** camera of its own instead of the player's eyes, without the weapon. What
** it changes -- enemies brought close, turned to the lens and posed on a
** frame of their own animations, others and shots hidden, BJ's figure where
** the player stands, the lens's field of view, and what the renderer marks
** as seen (also on the automap) -- is applied in IOSCamera_RenderBegin and
** put back exactly in IOSCamera_RenderEnd, every frame: nothing else in the
** game ever sees it.
**
** Lengths are tiles (a wall is 1 wide and 1 high; the player's eyes are
** 0.5 above the floor); angles are radians, as ECWolf's: 0 east, turning
** left, its y pointing south (forward is cos, -sin). Heights on screen go
** by ECWolf's own projection (heightnumerator): a tile 1 away is P pixels
** high, P = heightnumerator / 1024.
*/

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <vector>
#include <algorithm>

#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_main.h"
#include "wl_draw.h"
#include "wl_net.h"
#include "id_vl.h"
#include "id_sd.h"
#include "am_map.h"
#include "gamemap.h"
#include "id_ca.h"
#include "actor.h"
#include "m_random.h"
#include "r_sprites.h"
#include "v_palette.h"
#include "v_video.h"
#include "textures/textures.h"
#include "colormatcher.h"
#include "thingdef/thingdef.h"
#include "g_shared/a_playerpawn.h"

#include "IOSCamera.h"
#include "IOSCapture.h"

extern fixed viewz;
extern int viewshift;

#define CAM_PI 3.14159265358979323846

// A burst: CAM_BURST_MAX photos, CAM_BURST_GAP apart
#define CAM_BURST_MAX 10
#define CAM_BURST_GAP 0.2
// A photo the game hasn't taken in this long is dropped (a menu came up)
#define CAM_PHOTO_TIMEOUT 0.5
// A selfie's or cinematic burst's last photo stays this long, and either
// gives up when its photos can't be taken for CAM_GIVE_UP
#define CAM_SHOW_TIME 0.8
#define CAM_GIVE_UP 4.0
// Selfies: the nearest CAM_GROUP_MAX enemies where the player is: in his
// area (or one joined to it by open doors) within CAM_AREA_RANGE, or in
// sight within CAM_SIGHT_RANGE
#define CAM_AREA_RANGE 14.0
#define CAM_SIGHT_RANGE 20.0
#define CAM_GROUP_MAX 12
// The player's eyes above the floor; how near the camera keeps to a wall
#define CAM_EYE 0.5
#define CAM_WALL_GAP 0.2
// Action Freeze: nearest and furthest from its focus, its zoom
#define CAM_FREEZE_NEAR 0.35
#define CAM_FREEZE_FAR 12.0
#define CAM_FREEZE_ZOOM 8.0
#define CAM_PITCH_MAX (40.0 * CAM_PI / 180.0)
#define CAM_LOW 0.06
#define CAM_HIGH 0.94
#define CINEMA_SHOTS 5
// One on another's shoulders stands this much higher (a wall is 1 high;
// the people about 0.75)
#define CAM_SHOULDERS 0.58

enum { STAGE_NONE = 0, STAGE_SELFIE, STAGE_FREEZE, STAGE_CINEMA };

#ifdef ECWOLF_IOSRIG
#define CAMLOG(...) do { printf("cam: " __VA_ARGS__); fflush(stdout); } while(0)
#else
#define CAMLOG(...) do {} while(0)
#endif

// ------------------------------------------------------------ small things

static double Now()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// (xorshift32: the game's own random numbers aren't touched)
static unsigned randState = 2463534242u;
static unsigned Rand()
{
	unsigned x = randState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return randState = x;
}
static double Rand01() { return (Rand() & 0xFFFFFF) / (double)0x1000000; }
static double RandIn(double a, double b) { return a + (b - a) * Rand01(); }

struct V2 { double x, y; };
static V2 v2(double x, double y) { V2 v = { x, y }; return v; }
static V2 operator+(V2 a, V2 b) { return v2(a.x + b.x, a.y + b.y); }
static V2 operator-(V2 a, V2 b) { return v2(a.x - b.x, a.y - b.y); }
static V2 operator*(V2 a, double k) { return v2(a.x * k, a.y * k); }
static double Len(V2 a) { return sqrt(a.x * a.x + a.y * a.y); }
static V2 Fwd(double yaw) { return v2(cos(yaw), -sin(yaw)); }
static V2 Right(double yaw) { return v2(sin(yaw), cos(yaw)); }
static double Dot(V2 a, V2 b) { return a.x * b.x + a.y * b.y; }
static double YawTo(V2 from, V2 to) { return atan2(-(to.y - from.y), to.x - from.x); }
static double Wrap(double a) { while(a > CAM_PI) a -= 2 * CAM_PI; while(a < -CAM_PI) a += 2 * CAM_PI; return a; }
static uint32_t ToAngle(double a) { return (uint32_t)(int64_t)llround(a / (2 * CAM_PI) * 4294967296.0); }
static double FromAngle(uint32_t a) { return a * (2 * CAM_PI / 4294967296.0); }
static V2 PosOf(const AActor *a) { return v2(a->x / 65536.0, a->y / 65536.0); }

// The settings
static int optKinds = IOSCAPTURE_CLEAN, optShots = 3, optFreeze = 1;
extern "C" int IOSCamera_GetKinds(void) { return optKinds; }
extern "C" void IOSCamera_SetKinds(int k) { k &= IOSCAPTURE_ALL; optKinds = k ? k : IOSCAPTURE_CLEAN; }
extern "C" int IOSCamera_GetSelfieShots(void) { return optShots; }
extern "C" void IOSCamera_SetSelfieShots(int n) { optShots = (n == 1 || n == 5) ? n : 3; }
extern "C" int IOSCamera_GetFreezeOn(void) { return optFreeze; }
extern "C" void IOSCamera_SetFreezeOn(int on) { optFreeze = on != 0; }

static player_t *Player()
{
	player_t *p = &players[ConsolePlayer];
	return p->mo ? p : NULL;
}

// ------------------------------------------------------------ the level

// Whether a tile stops the camera and lines of sight: a wall, or a door not
// fully open
static bool Solid(int tx, int ty)
{
	if(!map || tx < 0 || ty < 0 || (unsigned)tx >= map->GetHeader().width || (unsigned)ty >= map->GetHeader().height)
		return true;
	MapSpot s = map->GetSpot(tx, ty, 0);
	if(!s->tile)
		return false;
	for(int i = 0;i < 4;i++)
		if(s->slideAmount[i] >= 0xffff)
			return false;
	return true;
}
static bool SolidAt(V2 p) { return Solid((int)floor(p.x), (int)floor(p.y)); }

// How far from a towards b before a solid tile (the whole way if clear)
static double ClearDist(V2 a, V2 b)
{
	const V2 d = b - a;
	const double len = Len(d);
	if(SolidAt(a))
		return 0;
	const int steps = (int)ceil(len / 0.05);
	for(int i = 1;i <= steps;i++)
	{
		const double t = len * i / steps;
		if(SolidAt(a + d * (t / len)))
			return fmax(0.0, t - 0.05);
	}
	return len;
}
static bool ClearLine(V2 a, V2 b) { return ClearDist(a, b) >= Len(b - a) - 1e-6; }

// Room round p: open, r clear of walls
static bool Roomy(V2 p, double r)
{
	if(SolidAt(p))
		return false;
	static const double k = 0.7071;
	const V2 off[8] = { v2(r, 0), v2(-r, 0), v2(0, r), v2(0, -r), v2(r * k, r * k), v2(-r * k, r * k), v2(r * k, -r * k), v2(-r * k, -r * k) };
	for(int i = 0;i < 8;i++)
		if(SolidAt(p + off[i]))
			return false;
	return true;
}

// ------------------------------------------------------------ the view

struct CamView
{
	V2 eye;
	double h;      // eyes above the floor
	double yaw;
	double tilt;   // looking down: heights per tile ahead (negative: up)
	double zoom;   // the lens (1: none)
};

static bool camOn = false;      // the photo view draws the 3D view
static CamView cam;
static bool showFigure = false; // BJ drawn where the player stands
static int stage = STAGE_NONE;

// The view's shape: tangents of half its width and height at zoom 1
static double TanH() { return scale > 0 ? (double)centerx / scale : 1.0; }
static double PixPerTile() { return heightnumerator / 1024.0; }
static double TanV() { return PixPerTile() > 0 ? (viewheight * 0.5) / PixPerTile() : 0.5; }

bool IOSCamera_View(int32_t &x, int32_t &y, uint32_t &angle)
{
	if(!camOn)
		return false;
	x = (int32_t)lround(cam.eye.x * 65536.0);
	y = (int32_t)lround(cam.eye.y * 65536.0);
	angle = ToAngle(cam.yaw);
	return true;
}

void IOSCamera_ViewHeight(int &shift, int32_t &vz)
{
	if(!camOn)
		return;
	vz = -(int32_t)lround(cam.h * 64.0 * 65536.0);
	// (ECWolf tilts by shearing the picture: kept to a third of its height)
	const double maxShift = viewheight / 3.0;
	shift = (int)lround(std::max(-maxShift, std::min(maxShift, cam.tilt * PixPerTile())));
}

uint32_t IOSCamera_RotateAngle(uint32_t playerAngle)
{
	return camOn ? ToAngle(cam.yaw) : playerAngle;
}

bool IOSCamera_HidesWeapon() { return stage != STAGE_NONE; }
bool IOSCamera_HoldsGame() { return stage != STAGE_NONE; }
extern "C" int IOSCamera_Stage(void) { return stage; }
extern "C" int IOSCamera_IsFrozen(void) { return stage == STAGE_FREEZE; }

// ------------------------------------------------------------ people

struct Person
{
	AActor *act;
	double dist;
	// the pose: where, how high, which frame (NULL: not posed: hidden)
	bool posed;
	V2 at;
	double lift;
	const Frame *frame;
	double yawFix; // (a little turn off the lens)
};
static std::vector<Person> people;
static std::vector<AActor *> hidden;

static bool Alive(const AActor *a)
{
	return (a->flags & FL_ISMONSTER) && (a->flags & FL_SHOOTABLE) && a->health > 0;
}

static bool FrameShows(const Frame *f)
{
	return f && R_CheckSpriteValid(f->spriteInf) && R_GetFrameTexture(f) != NULL;
}

// Its standing frame, or now and then a step of its walk (never an attack,
// pain or death frame: Spawn and See only)
static const Frame *PoseFrame(AActor *a, bool step)
{
	if(step)
	{
		const Frame *f = a->SeeState;
		if(FrameShows(f))
		{
			if(Rand01() < 0.5 && f->next && f->next->spriteInf == f->spriteInf && FrameShows(f->next))
				f = f->next;
			return f;
		}
	}
	if(FrameShows(a->SpawnState))
		return a->SpawnState;
	if(FrameShows(a->SeeState))
		return a->SeeState;
	return FrameShows(a->state) ? a->state : NULL;
}

static bool StillThere(const AActor *a)
{
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
		if((AActor *)it == a)
			return true;
	return false;
}

// ------------------------------------------------------------ render: apply, put back

struct Kept
{
	AActor *act;
	fixed x, y, z;
	angle_t angle;
	const Frame *state;
	unsigned int sprite;
};
static std::vector<Kept> kept;
struct KeptOut { AActor *act; short viewx; word viewheight; fixed transx, transy; };
static std::vector<KeptOut> keptOut;
static std::vector<unsigned char> keptVis;
static std::vector<unsigned int> keptAm;
static float keptFov = 0;
static bool fovChanged = false;
static bool applied = false;

static void Keep(AActor *a)
{
	Kept k = { a, a->x, a->y, a->z, a->angle, a->state, a->sprite };
	kept.push_back(k);
}

static const Frame *figureRun = NULL, *figureJump = NULL;
static bool figureJumping = false; // (the cinematic burst: the victory jump)
static bool figureLooked = false;

// BJ's figure: the victory run's frames (the player's own Wolfenstein 3D
// files; none in Spear of Destiny)
static void FindFigure()
{
	if(figureLooked)
		return;
	figureLooked = true;
	const ClassDef *cls = ClassDef::FindClass("BJRun");
	if(!cls)
		return;
	const Frame *run = cls->FindState(NAME_Spawn);
	const Frame *jump = cls->FindState(NAME_Death);
	if(FrameShows(run))
		figureRun = run; // (running towards the camera)
	// the jump's last frame: arms up
	const Frame *f = jump, *last = NULL;
	for(int i = 0;f && i < 8;i++, f = f->next)
	{
		if(FrameShows(f))
			last = f;
		if(f->duration < 0 || f->next == f)
			break;
	}
	figureJump = last;
	CAMLOG("figure: run %s, jump %s\n", figureRun ? "yes" : "no", figureJump ? "yes" : "no");
}

static void SetPose(AActor *a, V2 at, double lift, double yaw, const Frame *f)
{
	a->x = (fixed)lround(at.x * 65536.0);
	a->y = (fixed)lround(at.y * 65536.0);
	a->z = (fixed)lround(lift * 65536.0);
	a->angle = ToAngle(yaw);
	if(f)
	{
		a->state = f;
		a->sprite = f->spriteInf;
	}
}

void IOSCamera_RenderBegin()
{
	applied = false;
	if(!camOn || !map)
		return;
	player_t *p = Player();
	if(!p)
		return;
	applied = true;
	// what the renderer marks as seen, and on the automap
	const unsigned int n = map->GetHeader().width * map->GetHeader().height;
	keptVis.clear();
	keptAm.clear();
	for(unsigned int pl = 0;pl < map->NumPlanes();pl++)
	{
		for(unsigned int i = 0;i < n;i++)
		{
			const MapSpot s = &map->GetPlane(pl).map[i];
			keptVis.push_back(s->visible);
			keptAm.push_back(s->amFlags);
		}
	}
	// every actor's projection (the renderer's own, recomputed each frame)
	keptOut.clear();
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
	{
		AActor *a = it;
		KeptOut k = { a, a->viewx, a->viewheight, a->transx, a->transy };
		keptOut.push_back(k);
	}
	kept.clear();
	// the people posed, or hidden
	for(size_t i = 0;i < people.size();i++)
	{
		Person &q = people[i];
		Keep(q.act);
		if(q.posed)
			SetPose(q.act, q.at, q.lift, YawTo(q.at, cam.eye) + q.yawFix, q.frame);
		else
			q.act->sprite = SPR_NONE;
	}
	for(size_t i = 0;i < hidden.size();i++)
	{
		Keep(hidden[i]);
		hidden[i]->sprite = SPR_NONE;
	}
	// BJ (his sprites face the camera from every side): running, or in the
	// cinematic burst his victory jump
	AActor *mo = p->mo;
	Keep(mo);
	if(showFigure)
	{
		FindFigure();
		const Frame *f = (figureJumping && figureJump) ? figureJump : (figureRun ? figureRun : figureJump);
		if(f)
		{
			mo->state = f;
			mo->sprite = f->spriteInf;
			mo->z = 0;
		}
		else
			mo->sprite = SPR_NONE;
	}
	else
		mo->sprite = SPR_NONE;
	// the lens
	fovChanged = false;
	if(fabs(cam.zoom - 1.0) > 0.0001)
	{
		keptFov = p->FOV;
		p->FOV = (float)(keptFov / cam.zoom);
		CalcProjection(mo->radius);
		fovChanged = true;
	}
}

void IOSCamera_RenderEnd()
{
	if(!applied)
		return;
	applied = false;
	player_t *p = Player();
	if(fovChanged && p)
	{
		p->FOV = keptFov;
		CalcProjection(p->mo->radius);
		fovChanged = false;
	}
	for(size_t i = kept.size();i-- > 0;)
	{
		Kept &k = kept[i];
		k.act->x = k.x;
		k.act->y = k.y;
		k.act->z = k.z;
		k.act->angle = k.angle;
		k.act->state = k.state;
		k.act->sprite = k.sprite;
	}
	kept.clear();
	for(size_t i = 0;i < keptOut.size();i++)
	{
		KeptOut &k = keptOut[i];
		k.act->viewx = k.viewx;
		k.act->viewheight = k.viewheight;
		k.act->transx = k.transx;
		k.act->transy = k.transy;
	}
	keptOut.clear();
	const unsigned int n = map->GetHeader().width * map->GetHeader().height;
	size_t j = 0;
	for(unsigned int pl = 0;pl < map->NumPlanes();pl++)
	{
		for(unsigned int i = 0;i < n && j < keptVis.size();i++, j++)
		{
			MapSpot s = &map->GetPlane(pl).map[i];
			s->visible = keptVis[j] != 0;
			s->amFlags = keptAm[j];
		}
	}
}

// ------------------------------------------------------------ the player's eyes

static V2 eyeAt;    // the player's eyes (as the game draws them)
static double eyeYaw;

static bool ReadEye()
{
	player_t *p = Player();
	if(!p)
		return false;
	eyeAt = PosOf(p->mo);
	eyeYaw = FromAngle(p->mo->angle);
	return true;
}

// How far ahead along yaw before a wall (at most max)
static double FreeAhead(V2 from, double yaw, double max)
{
	return ClearDist(from, from + Fwd(yaw) * max);
}

// Whether a is where the player is: in his area or one joined to it by
// open doors (ECWolf's sound zones), within CAM_AREA_RANGE, or in sight
// within CAM_SIGHT_RANGE
static bool HereWithPlayer(AActor *a, double dist)
{
	const MapZone *mine = Player()->mo->GetZone(), *theirs = a->GetZone();
	if(dist <= CAM_AREA_RANGE && mine && theirs && (mine == theirs || map->CheckLink(mine, theirs, true)))
		return true;
	return dist <= CAM_SIGHT_RANGE && ClearLine(eyeAt, PosOf(a));
}

// The living enemies, nearest first (here: only those where the player is)
static void FindPeople(double range, size_t max, bool here = false)
{
	people.clear();
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
	{
		AActor *a = it;
		if(!Alive(a) || a == Player()->mo)
			continue;
		Person q;
		memset(&q, 0, sizeof(q));
		q.act = a;
		q.dist = Len(PosOf(a) - eyeAt);
		if(q.dist > range || (here && !HereWithPlayer(a, q.dist)))
			continue;
		people.push_back(q);
	}
	std::sort(people.begin(), people.end(), [](const Person &a, const Person &b) { return a.dist < b.dist; });
	if(people.size() > max)
		people.resize(max);
}

// ------------------------------------------------------------ selfies

enum { THEME_HUDDLE = 0, THEME_PEEK, THEME_CLASS, THEME_CORNER, THEME_TINY, THEME_TOTEM, THEME_PYRAMID, THEME_LIFT, THEMES };
static const char *const themeNames[THEMES] = { "huddle", "peek", "class", "corner", "tiny", "totem", "pyramid", "lift" };
static const int themeWeights[THEMES] = { 18, 14, 14, 10, 10, 10, 10, 10 };
static const int themeLeast[THEMES] = { 1, 3, 3, 2, 3, 2, 3, 3 };
static int lastTheme = -1;
static double groupYaw = 0, groupRoom = 0;
static int ctTaken = 0, ctShots = 0;
static double tStage = 0, tShown = 0, tWaited = 0;
static bool asked = false;
static const char *cantWhy = NULL;

// The way to take it: ahead if there is room, else the roomiest way round
static bool PickGroupHeading()
{
	double best = -1;
	for(int i = 0;i < 16;i++)
	{
		const int k = (i + 1) / 2;
		const double yaw = eyeYaw + ((i & 1) ? 1 : -1) * k * (2 * CAM_PI / 16);
		const double room = FreeAhead(eyeAt, yaw, 8.0);
		const V2 mid = eyeAt + Fwd(yaw) * fmin(room * 0.6, 2.0);
		const double side = fmin(ClearDist(mid, mid + Right(yaw) * 1.5), ClearDist(mid, mid - Right(yaw) * 1.5));
		const double score = fmin(room, 5.0) + side * 0.6 - k * 0.12;
		if(i == 0 && room >= 3.0)
		{
			groupYaw = yaw;
			groupRoom = room;
			return true;
		}
		if(score > best)
		{
			best = score;
			groupYaw = yaw;
			groupRoom = room;
		}
	}
	return groupRoom >= 1.2;
}

static bool NearSolidThing(V2 p, double r)
{
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
	{
		AActor *a = it;
		if(!(a->flags & FL_SOLID) || (a->flags & FL_ISMONSTER) || a == Player()->mo)
			continue;
		if(Len(PosOf(a) - p) < r)
			return true;
	}
	return false;
}

// Person i posed at (lateral, depth) in the camera's view, lift above the
// floor: nudged in, out or to the middle where that spot has no room
static bool PlaceAt(size_t i, double lat, double depth, double lift, bool step = false)
{
	if(i >= people.size())
		return false;
	static const double dk[] = { 1.0, 0.88, 1.14, 0.76, 1.3 };
	static const double lk[] = { 1.0, 0.7, 0.4 };
	const V2 f = Fwd(cam.yaw), r = Right(cam.yaw);
	for(int a = 0;a < 3;a++)
	{
		for(int b = 0;b < 5;b++)
		{
			const double d = depth * dk[b];
			if(d < 0.75 || d > groupRoom - 0.3)
				continue;
			const V2 p = cam.eye + f * d + r * (lat * lk[a]);
			if(!Roomy(p, 0.22) || !ClearLine(cam.eye, p) || NearSolidThing(p, 0.45))
				continue;
			Person &q = people[i];
			q.posed = true;
			q.at = p;
			q.lift = lift;
			q.frame = PoseFrame(q.act, step || Rand01() < 0.15);
			q.yawFix = RandIn(-0.25, 0.25);
			if(!q.frame)
				q.posed = false;
			return q.posed;
		}
	}
	return false;
}

// A loose row (n people from first) across the view at depth
static void Row(size_t first, size_t n, double depth, double spacing, double lift, double latMid = 0)
{
	for(size_t j = 0;j < n;j++)
		PlaceAt(first + j, latMid + (j - (n - 1) * 0.5) * spacing + RandIn(-0.05, 0.05), depth + RandIn(-0.12, 0.12), lift);
}

static int PickTheme(size_t n)
{
	int total = 0;
	for(int t = 0;t < THEMES;t++)
		if(n >= (size_t)themeLeast[t] && t != lastTheme)
			total += themeWeights[t];
	int r = total > 0 ? (int)(Rand() % total) : 0;
	for(int t = 0;t < THEMES;t++)
	{
		if(n < (size_t)themeLeast[t] || t == lastTheme)
			continue;
		if((r -= themeWeights[t]) < 0)
			return t;
	}
	return THEME_HUDDLE;
}

static void Theme(int t)
{
	const size_t n = people.size();
	const double tanH = TanH();
	cam.h = CAM_EYE;
	cam.tilt = 0;
	cam.zoom = 1.0;
	switch(t)
	{
	default:
	case THEME_HUDDLE:
	{
		const size_t front = std::min<size_t>(n, 4), back = std::min<size_t>(n - front, 4);
		const double d = RandIn(1.5, 1.9);
		Row(0, front, d, 0.42, 0);
		Row(front, back, d + 0.5, 0.42, 0.06);
		break;
	}
	case THEME_PEEK:
	{
		const double d = 1.15;
		PlaceAt(0, -d * tanH * 0.8, d, 0);
		PlaceAt(1, d * tanH * 0.8, d, 0);
		Row(2, std::min<size_t>(n - 2, 4), 2.2, 0.45, 0);
		break;
	}
	case THEME_CLASS:
	{
		const size_t front = std::min<size_t>((n + 1) / 2, 4), back = std::min<size_t>(n - front, 5);
		Row(0, front, 1.8, 0.5, 0);
		Row(front, back, 2.6, 0.5, 0.2);
		break;
	}
	case THEME_CORNER:
	{
		const double side = Rand01() < 0.5 ? -1 : 1, d = 2.0;
		const size_t m = std::min<size_t>(n, 5);
		for(size_t j = 0;j < m;j++)
			PlaceAt(j, side * (d * tanH * 0.55 - j * 0.28), d + j * 0.25 + RandIn(-0.1, 0.1), 0);
		break;
	}
	case THEME_TINY:
	{
		const double far = fmin(groupRoom - 0.4, 6.5), d = 1.15;
		PlaceAt(2, RandIn(-0.2, 0.2), far, 0);
		PlaceAt(0, -d * tanH * 0.6, d, 0);
		PlaceAt(1, d * tanH * 0.6, d, 0);
		break;
	}
	case THEME_TOTEM:
	{
		const double d = 2.6;
		const size_t m = std::min<size_t>(n, 3);
		double top = 0;
		for(size_t j = 0;j < m;j++)
			if(PlaceAt(j, 0, d, j * CAM_SHOULDERS))
				top = j * CAM_SHOULDERS;
		// (the first's place: the others stand on it, exactly)
		for(size_t j = 1;j < m;j++)
			if(people[j].posed && people[0].posed)
				people[j].at = people[0].at + Fwd(cam.yaw) * (0.02 * j);
		if(n > 3) PlaceAt(3, -0.65, d, 0);
		if(n > 4) PlaceAt(4, 0.65, d, 0);
		cam.h = 0.8;
		cam.zoom = 0.8;
		cam.tilt = (cam.h - (top + 0.75) * 0.5) / d;
		break;
	}
	case THEME_PYRAMID:
	{
		const double d = 3.0;
		static const double lat[6] = { -0.45, 0, 0.45, -0.225, 0.225, 0 };
		static const double lift[6] = { 0, 0, 0, CAM_SHOULDERS, CAM_SHOULDERS, 2 * CAM_SHOULDERS };
		const size_t m = std::min<size_t>(n, 6);
		double top = 0;
		for(size_t j = 0;j < m;j++)
		{
			if(PlaceAt(j, lat[j], d, lift[j]))
				top = fmax(top, lift[j]);
		}
		// (all on one line across: the upper ones on the shoulders below)
		if(people[0].posed || people[1].posed)
		{
			const Person &base = people[1].posed ? people[1] : people[0];
			const double baseDepth = Dot(base.at - cam.eye, Fwd(cam.yaw));
			for(size_t j = 0;j < m;j++)
				if(people[j].posed)
					people[j].at = cam.eye + Fwd(cam.yaw) * (baseDepth + 0.02 * (j / 3 + (j >= 5))) + Right(cam.yaw) * lat[j];
		}
		cam.h = 0.85;
		cam.zoom = 0.75;
		cam.tilt = (cam.h - (top + 0.75) * 0.5) / d;
		break;
	}
	case THEME_LIFT:
	{
		const size_t up = Rand() % std::min<size_t>(n, 3);
		PlaceAt(up, 0, 2.1, 0.5);
		size_t k = 0;
		for(size_t j = 0;j < std::min<size_t>(n, 6);j++)
		{
			if(j == up)
				continue;
			PlaceAt(j, ((k & 1) ? 1 : -1) * (0.25 + 0.42 * (k / 2)), 1.8 + RandIn(-0.1, 0.1), 0);
			k++;
		}
		cam.tilt = -0.12;
		break;
	}
	}
}

// Everyone else hidden: the enemies not in it, the shots, and whatever
// stands in front of a face
static void HideOthers()
{
	hidden.clear();
	const V2 f = Fwd(cam.yaw), r = Right(cam.yaw);
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
	{
		AActor *a = it;
		if(a == Player()->mo || a->sprite == SPR_NONE)
			continue;
		bool inGroup = false;
		for(size_t i = 0;i < people.size() && !inGroup;i++)
			inGroup = people[i].act == a;
		if(inGroup)
			continue;
		bool hide = (a->flags & FL_ISMONSTER) || (a->flags & FL_MISSILE);
		if(!hide)
		{
			const V2 d = PosOf(a) - cam.eye;
			const double depth = Dot(d, f), lat = Dot(d, r);
			for(size_t i = 0;i < people.size() && !hide;i++)
			{
				const Person &q = people[i];
				if(!q.posed)
					continue;
				const double qd = Dot(q.at - cam.eye, f), ql = Dot(q.at - cam.eye, r);
				hide = depth > 0 && depth < qd + 0.1 && fabs(lat - ql) < 0.55;
			}
		}
		if(hide)
			hidden.push_back(a);
	}
}

// ------------------------------------------------------------ BJ in his selfies

// He holds the phone: his status-bar face (the player's own data: healthy,
// the grin, Spear's golden eyes) big in a bottom corner, scaled up a whole
// number of times (nearest), the background keyed out, on shoulders in his
// grey shirt drawn in the same pixels (the victory run's shirt), cropped by
// the frame's edges. Drawn over the photo view's 3D view each frame (the
// screen is drawn afresh every frame: nothing of the game changes).
#define SELF_W 24
#define SELF_H 32
#define SELF_NECK0 4  // (the neck's columns at the picture's bottom)
#define SELF_NECK1 20
#define SELF_LEFT -14 // (the shirt's columns, past the picture's sides)
#define SELF_RIGHT 37
#define SELF_ROWS 26  // (rows below the picture, at most)

struct SelfPic
{
	bool ok;
	BYTE px[SELF_H][SELF_W];
	bool on[SELF_H][SELF_W];
};
static const char *const selfFaces[] = { "STFST00", "STFEVL0", "STFGOD0", "STFST01", "STFST02" };
static const int selfFaceWeights[] = { 10, 8, 8, 3, 3 };
#define SELF_FACES (int)(sizeof(selfFaces) / sizeof(selfFaces[0]))
static SelfPic selfPics[SELF_FACES];
static bool selfLooked[SELF_FACES];
static BYTE shirtLight, shirtMid, shirtDark, shirtDeep, shirtLine;

struct SelfShot
{
	bool on;
	int face;    // selfFaces[]
	int k;       // view pixels a picture pixel
	int x0, y0;  // the picture's top left in the view
	bool flip;
	int side;    // -1: bottom left, 1: bottom right
};
static SelfShot self;

static bool LoadSelfPic(int i)
{
	SelfPic &f = selfPics[i];
	if(selfLooked[i])
		return f.ok;
	selfLooked[i] = true;
	f.ok = false;
	const FTextureID id = TexMan.CheckForTexture(selfFaces[i], FTexture::TEX_Any);
	FTexture *tex = id.isValid() ? TexMan[id] : NULL;
	if(!tex || tex->GetWidth() != SELF_W || tex->GetHeight() != SELF_H)
		return false;
	int raw[SELF_H][SELF_W];
	for(int y = 0;y < SELF_H;y++)
		for(int x = 0;x < SELF_W;x++)
			raw[y][x] = -1;
	for(int x = 0;x < SELF_W;x++)
	{
		const FTexture::Span *sp;
		const BYTE *col = tex->GetColumn(x, &sp);
		for(;sp && sp->Length;sp++)
			for(int y = sp->TopOffset;y < sp->TopOffset + sp->Length && y < SELF_H;y++)
				raw[y][x] = col[y];
	}
	// the background: the colours most of the border is (the panel's grey,
	// the blue line over it), and on the border anything near them, keyed
	// out from the edges inwards
	int count[256] = { 0 }, border = 0;
	for(int y = 0;y < SELF_H;y++)
		for(int x = 0;x < SELF_W;x++)
			if((y == 0 || x == 0 || y == SELF_H - 1 || x == SELF_W - 1) && raw[y][x] >= 0)
			{
				count[raw[y][x]]++;
				border++;
			}
	bool bg[256];
	for(int c = 0;c < 256;c++)
		bg[c] = count[c] * 20 >= border && count[c] > 0;
	bool nearBg[256];
	for(int c = 0;c < 256;c++)
	{
		nearBg[c] = bg[c];
		for(int b = 0;b < 256 && !nearBg[c];b++)
		{
			if(!bg[b])
				continue;
			const int dr = GPalette.BaseColors[c].r - GPalette.BaseColors[b].r;
			const int dg = GPalette.BaseColors[c].g - GPalette.BaseColors[b].g;
			const int db = GPalette.BaseColors[c].b - GPalette.BaseColors[b].b;
			nearBg[c] = dr * dr + dg * dg + db * db < 48 * 48;
		}
	}
	bool out[SELF_H][SELF_W];
	std::vector<int> todo;
	for(int y = 0;y < SELF_H;y++)
		for(int x = 0;x < SELF_W;x++)
		{
			out[y][x] = raw[y][x] < 0;
			const bool edge = y == 0 || x == 0 || y == SELF_H - 1 || x == SELF_W - 1;
			if(!out[y][x] && edge && nearBg[raw[y][x]] && y < SELF_H - 1)
			{
				out[y][x] = true;
				todo.push_back(y * SELF_W + x);
			}
		}
	while(!todo.empty())
	{
		const int y = todo.back() / SELF_W, x = todo.back() % SELF_W;
		todo.pop_back();
		static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
		for(int d = 0;d < 4;d++)
		{
			const int nx = x + dx[d], ny = y + dy[d];
			if(nx < 0 || ny < 0 || nx >= SELF_W || ny >= SELF_H || out[ny][nx] || !bg[raw[ny][nx]])
				continue;
			out[ny][nx] = true;
			todo.push_back(ny * SELF_W + nx);
		}
	}
	int shown = 0;
	for(int y = 0;y < SELF_H;y++)
		for(int x = 0;x < SELF_W;x++)
		{
			f.on[y][x] = !out[y][x];
			f.px[y][x] = raw[y][x] < 0 ? 0 : (BYTE)raw[y][x];
			shown += f.on[y][x];
		}
	f.ok = shown > SELF_W * SELF_H / 3;
	if(!shirtLine)
	{
		shirtLight = ColorMatcher.Pick(178, 178, 178);
		shirtMid = ColorMatcher.Pick(142, 142, 142);
		shirtDark = ColorMatcher.Pick(113, 113, 113);
		shirtDeep = ColorMatcher.Pick(85, 85, 85);
		shirtLine = ColorMatcher.Pick(44, 44, 44);
	}
	CAMLOG("self: %s %s (%d of %d shown)\n", selfFaces[i], f.ok ? "yes" : "no", shown, SELF_W * SELF_H);
	return f.ok;
}

// This photo's BJ: which face, how big, which corner, how far off the edge
static void PickSelf()
{
	self.on = false;
	int total = 0;
	for(int i = 0;i < SELF_FACES;i++)
		if(LoadSelfPic(i))
			total += selfFaceWeights[i];
	if(total <= 0 || viewheight <= 0 || viewwidth <= 0)
		return;
	int r = (int)(Rand() % total);
	self.face = 0;
	for(int i = 0;i < SELF_FACES;i++)
	{
		if(!selfPics[i].ok)
			continue;
		if((r -= selfFaceWeights[i]) < 0)
		{
			self.face = i;
			break;
		}
	}
	// the face 50-80% of the view's height; shoulders showing below it
	const double frac = RandIn(0.5, 0.8);
	self.k = std::max(2, (int)lround(frac * viewheight / SELF_H));
	const int shirt = 4 + (int)(Rand() % 8);
	self.y0 = viewheight - (SELF_H + shirt) * self.k;
	if(self.y0 < viewheight / 20)
		self.y0 = viewheight / 20;
	self.side = (Rand() & 1) ? 1 : -1;
	self.flip = (Rand() & 1) != 0;
	// up to a third of the face off the frame's edge
	const int off = (int)(Rand() % 8);
	self.x0 = self.side < 0 ? -off * self.k : viewwidth - (SELF_W - off) * self.k;
	self.on = true;
	CAMLOG("self: %s, %d x, %s, %d off the edge, %d of the shirt\n", selfFaces[self.face], self.k, self.side < 0 ? "left" : "right", off, shirt);
}

// The shirt's colour at picture pixel (x, y) (y >= SELF_H - 4), or -1:
// none there. Shoulders sloping from the neck, a V at the collar, a light
// rim on top, darker round the sides.
static int ShirtAt(int x, int y)
{
	const int neckL = SELF_NECK0, neckR = SELF_NECK1;
	const int dist = x < neckL ? neckL - x : x > neckR ? x - neckR : 0;
	if(dist > 15)
		return -1;
	// the top of the shoulder (and, near the neck, the collar)
	const double top = dist == 0 ? SELF_H : SELF_H - 2.5 + 0.45 * dist + 0.05 * dist * dist;
	const int yt = (int)floor(top);
	if(y < yt)
		return -1;
	// the V of the collar
	const int mid = (neckL + neckR) / 2;
	const int vHalf = 6 - 2 * (y - SELF_H);
	if(dist == 0 && abs(x - mid) < vHalf)
		return -2; // (the neck's skin)
	if(dist == 0 && abs(x - mid) == vHalf)
		return shirtLine;
	// the arm's outside edge
	if(dist == 15)
		return shirtLine;
	if(y == yt)
		return shirtLine;
	if(y == yt + 1 && dist < 11)
		return shirtLight;
	if(dist >= 12 || (dist == 11 && y > yt + 1))
		return shirtDeep;
	if(dist >= 8 || (x == mid && y >= SELF_H + 4))
		return shirtDark;
	return shirtMid;
}

static void DrawSelf(unsigned char *vbuf, unsigned pitch, int w, int h)
{
	if(!self.on || !selfPics[self.face].ok)
		return;
	const SelfPic &f = selfPics[self.face];
	const int k = self.k;
	const int u0 = std::max(SELF_LEFT, (int)floor((double)-self.x0 / k));
	const int u1 = std::min(SELF_RIGHT, (int)ceil((double)(w - self.x0) / k));
	const int v1 = std::min(SELF_H + SELF_ROWS, (int)ceil((double)(h - self.y0) / k));
	for(int v = std::max(0, -self.y0 / k);v < v1;v++)
	{
		for(int u = u0;u < u1;u++)
		{
			// (the picture's own column, mirrored or not)
			const int pu = self.flip ? SELF_W - 1 - u : u;
			int c = -1;
			if(v < SELF_H && pu >= 0 && pu < SELF_W && f.on[v][pu])
				c = f.px[v][pu];
			if(v >= SELF_H - 4)
			{
				const int sc = ShirtAt(pu, v);
				if(sc == -2)
				{
					// the neck runs on under the picture, its bottom row's colours
					if(v >= SELF_H && pu >= 0 && pu < SELF_W)
						c = f.px[SELF_H - 1][pu];
				}
				else if(sc >= 0 && (c < 0 || v >= SELF_H - 1 || (pu < SELF_NECK0 || pu > SELF_NECK1)))
					c = sc;
			}
			if(c < 0)
				continue;
			const int x0 = std::max(0, self.x0 + u * k), x1 = std::min(w, self.x0 + (u + 1) * k);
			const int y0 = std::max(0, self.y0 + v * k), y1 = std::min(h, self.y0 + (v + 1) * k);
			for(int y = y0;y < y1;y++)
				memset(vbuf + (size_t)y * pitch + x0, c, x1 > x0 ? x1 - x0 : 0);
		}
	}
}

// The view turned towards BJ's side, so the group stands clear of him
static void SelfShift()
{
	if(!self.on || viewwidth <= 0)
		return;
	const int edge = self.side < 0 ? self.x0 + SELF_W * self.k : viewwidth - self.x0;
	const double frac = std::min(0.3, (double)edge / viewwidth * 0.5);
	cam.yaw += -self.side * atan(2 * frac * TanH());
}

static bool Pose()
{
	int t = PickTheme(people.size());
	for(int attempt = 0;attempt < 3;attempt++)
	{
		for(size_t i = 0;i < people.size();i++)
			people[i].posed = false;
		cam.yaw = groupYaw;
		Theme(t);
		size_t posed = 0;
		for(size_t i = 0;i < people.size();i++)
			posed += people[i].posed;
		if(posed >= std::min<size_t>(people.size(), (size_t)themeLeast[t]))
		{
			lastTheme = t;
			PickSelf();
			SelfShift();
			HideOthers();
			CAMLOG("selfie %s: %d of %d posed, yaw %.0f, room %.1f\n", themeNames[t], (int)posed, (int)people.size(),
				cam.yaw * 180 / CAM_PI, groupRoom);
			return true;
		}
		t = attempt == 0 ? THEME_HUDDLE : THEME_CORNER;
	}
	return false;
}

static bool SelfieBegin()
{
	FindPeople(CAM_SIGHT_RANGE, CAM_GROUP_MAX, true);
	if(people.empty())
	{
		cantWhy = "No one around for a group photo";
		return false;
	}
	if(!PickGroupHeading())
	{
		cantWhy = "No room for a group photo here";
		return false;
	}
	cam.eye = eyeAt;
	cam.yaw = groupYaw;
	cam.h = CAM_EYE;
	cam.tilt = 0;
	cam.zoom = 1;
	camOn = true;
	showFigure = false;
	lastTheme = -1;
	if(!Pose())
	{
		cantWhy = "No room for a group photo here";
		return false;
	}
	return true;
}

// ------------------------------------------------------------ Action Freeze and the orbit

static V2 focus;
static double focusH = 0.45;
static double orbYaw, orbPitch, orbDist, orbDistNow, distStart;
static bool smooth = true;

// The camera where the orbit puts it, kept out of walls
static bool PlaceOrbit(bool snap)
{
	const double horizWant = orbDist * cos(orbPitch);
	const V2 back = Fwd(orbYaw) * -1.0;
	double room = ClearDist(focus, focus + back * (horizWant + CAM_WALL_GAP)) - CAM_WALL_GAP;
	// (and room round it: back off until it has)
	double d = fmin(room, horizWant);
	while(d > CAM_FREEZE_NEAR * 0.5 && !Roomy(focus + back * d, 0.12))
		d -= 0.05;
	d = fmax(d, CAM_FREEZE_NEAR * 0.5);
	if(snap || !smooth || d < orbDistNow)
		orbDistNow = d;
	else
		orbDistNow += (d - orbDistNow) * 0.2;
	const V2 eye = focus + back * orbDistNow;
	if(SolidAt(eye))
		return false;
	cam.eye = eye;
	cam.yaw = orbYaw;
	cam.h = std::max(CAM_LOW, std::min(CAM_HIGH, focusH + orbDistNow * tan(orbPitch)));
	cam.tilt = (cam.h - focusH) / std::max(orbDistNow, 0.2);
	camOn = true;
	// (BJ, unless the camera is in his head)
	showFigure = Len(cam.eye - eyeAt) > 0.4;
	return true;
}

// What the player was looking at: an enemy near the middle of the view, or
// where the view meets the level
static void FindFocus()
{
	FindPeople(14.0, 64);
	const V2 f = Fwd(eyeYaw);
	for(size_t i = 0;i < people.size();i++)
	{
		const V2 p = PosOf(people[i].act);
		const double ang = fabs(Wrap(YawTo(eyeAt, p) - eyeYaw));
		if(ang < 0.2 && people[i].dist > 0.6 && ClearLine(eyeAt, p))
		{
			focus = p;
			focusH = 0.45;
			orbDist = Len(p - eyeAt);
			people.clear();
			return;
		}
	}
	people.clear();
	const double ahead = FreeAhead(eyeAt, eyeYaw, 6.0) - 0.4;
	focusH = CAM_EYE;
	if(ahead < 0.8)
	{
		// (up against a wall: round BJ, from behind him)
		focus = eyeAt;
		orbDist = 1.5;
		return;
	}
	orbDist = std::max(0.8, std::min(ahead, 3.0));
	focus = eyeAt + f * orbDist;
}

static bool FreezeBegin()
{
	FindFocus();
	if(SolidAt(focus))
		return false;
	orbYaw = eyeYaw;
	orbPitch = 0;
	orbDistNow = orbDist;
	distStart = orbDist;
	cam.zoom = 1;
	return PlaceOrbit(true);
}

extern "C" void IOSCamera_FreezeOrbit(float yawDeg, float pitchDeg)
{
	if(stage != STAGE_FREEZE)
		return;
	const double y0 = orbYaw, p0 = orbPitch;
	const double k = 1.0 / std::max(cam.zoom, 1.0); // (zoomed in, a slide turns less)
	orbYaw += yawDeg * k * CAM_PI / 180;
	orbPitch = std::max(-CAM_PITCH_MAX, std::min(CAM_PITCH_MAX, orbPitch + pitchDeg * k * CAM_PI / 180));
	if(!PlaceOrbit(false))
	{
		orbYaw = y0;
		orbPitch = p0;
		PlaceOrbit(true);
	}
}

extern "C" void IOSCamera_FreezePan(float right, float down)
{
	if(stage != STAGE_FREEZE)
		return;
	// what is seen follows the fingers: the focus moves across the view
	const double span = 2.0 * std::max(orbDistNow, 0.3) * TanV() / std::max(cam.zoom, 1.0);
	const V2 to = focus - Right(orbYaw) * (right * span);
	const double room = ClearDist(focus, to);
	const double len = Len(to - focus);
	if(len > 1e-6)
	{
		const double k = std::max(0.0, std::min(1.0, (room - CAM_WALL_GAP) / len));
		focus = focus + (to - focus) * k;
	}
	focusH = std::max(0.05, std::min(0.95, focusH + down * span));
	PlaceOrbit(false);
}

extern "C" float IOSCamera_FreezeZoomNow(void)
{
	return stage == STAGE_FREEZE ? (float)cam.zoom : 1.0f;
}

extern "C" void IOSCamera_FreezeZoom(float factor)
{
	if(stage != STAGE_FREEZE || factor <= 0)
		return;
	// one scale: above 1 the lens's zoom; below, how far the camera has
	// backed off from where the freeze began
	double z = cam.zoom > 1.0 ? cam.zoom : distStart / std::max(orbDist, 0.01);
	z = std::max(distStart / CAM_FREEZE_FAR, std::min(CAM_FREEZE_ZOOM, z * factor));
	if(z >= 1.0)
	{
		cam.zoom = z;
		orbDist = distStart;
	}
	else
	{
		cam.zoom = 1.0;
		orbDist = distStart / z;
	}
	PlaceOrbit(false);
}

// ------------------------------------------------------------ the cinematic burst

static double cineYaw[CINEMA_SHOTS];

static bool CinemaBegin()
{
	FindPeople(10.0, 64);
	const Person *target = NULL;
	for(size_t i = 0;i < people.size() && !target;i++)
	{
		const V2 p = PosOf(people[i].act);
		if(fabs(Wrap(YawTo(eyeAt, p) - eyeYaw)) < 0.9 && ClearLine(eyeAt, p))
			target = &people[i];
	}
	double start, step;
	if(target)
	{
		const V2 p = PosOf(target->act);
		focus = (eyeAt + p) * 0.5;
		orbDist = std::max(2.2, std::min(Len(p - eyeAt) * 0.9, 6.0));
		start = 60 * CAM_PI / 180;
		step = 30 * CAM_PI / 180;
	}
	else
	{
		focus = eyeAt;
		orbDist = 2.0;
		start = 60 * CAM_PI / 180;
		step = 40 * CAM_PI / 180;
	}
	people.clear();
	focusH = 0.45;
	if(SolidAt(focus))
		return false;
	// the side with the most room
	double room[2] = { 0, 0 };
	for(int side = 0;side < 2;side++)
		for(int i = 0;i < 4;i++)
		{
			const double y = eyeYaw + (side ? -1 : 1) * (start + step * i);
			room[side] += std::min(ClearDist(focus, focus - Fwd(y) * orbDist), orbDist);
		}
	const int sgn = room[0] >= room[1] ? 1 : -1;
	for(int i = 0;i < 4;i++)
		cineYaw[i + 1] = eyeYaw + sgn * (start + step * i);
	orbPitch = 0.3;
	cam.zoom = 1;
	smooth = false;
	figureJumping = true;
	return true;
}

// Whatever stands between the camera and BJ (a lamp, a plant) left out of
// the cinematic burst's shot
static void HideBeforeFigure()
{
	hidden.clear();
	const V2 f = Fwd(cam.yaw), r = Right(cam.yaw);
	const double bd = Dot(eyeAt - cam.eye, f), bl = Dot(eyeAt - cam.eye, r);
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
	{
		AActor *a = it;
		if(a == Player()->mo || a->sprite == SPR_NONE || (a->flags & FL_ISMONSTER))
			continue;
		const V2 d = PosOf(a) - cam.eye;
		if(Dot(d, f) > 0 && Dot(d, f) < bd - 0.1 && fabs(Dot(d, r) - bl) < 0.45)
			hidden.push_back(a);
	}
}

static void CinemaPlace(int shot)
{
	if(shot == 0)
	{
		camOn = false; // (the first: what the player saw, without the weapon)
		return;
	}
	// (a little further round where the camera would be on top of BJ)
	for(int k = 0;k < 4;k++)
	{
		orbYaw = cineYaw[shot] + (k & 1 ? -1 : 1) * ((k + 1) / 2) * 0.35;
		if(PlaceOrbit(true) && orbDistNow >= std::min(1.2, orbDist * 0.7))
		{
			HideBeforeFigure();
			return;
		}
	}
	orbYaw = cineYaw[shot];
	PlaceOrbit(true);
	HideBeforeFigure();
}

// ------------------------------------------------------------ stages

static double tPhotoAsked = 0;
static int ctBurst = 0;
static double tBurstNext = 0;
static bool burst = false;

static void EndBurst()
{
	if(!burst)
		return;
	burst = false;
	IOSPhotos_SetBatch(0);
}

#ifdef ECWOLF_IOSRIG
static unsigned RigHash();
#endif

static void EndStage()
{
#ifdef ECWOLF_IOSRIG
	const bool bHash = stage != STAGE_NONE;
#endif
	if(stage == STAGE_FREEZE)
		IOSPhotos_FreezeShown(0);
	if(stage == STAGE_SELFIE || stage == STAGE_CINEMA)
		IOSPhotos_SetBatch(0);
	if(stage != STAGE_NONE)
		IOSCapture_NoteFreeze(Now() - tStage);
	stage = STAGE_NONE;
	camOn = false;
	showFigure = false;
	figureJumping = false;
	smooth = true;
	people.clear();
	hidden.clear();
	self.on = false;
#ifdef ECWOLF_IOSRIG
	if(bHash)
		CAMLOG("state hash after %08x\n", RigHash());
#endif
}

extern "C" void IOSCamera_FreezeEnd(void)
{
	if(stage != STAGE_FREEZE)
		return;
	IOSCapture_Cancel();
	EndStage();
}

extern "C" int IOSCamera_NumTaken(void) { return ctTaken; }

void IOSCamera_Abort()
{
	EndBurst();
	IOSCapture_Cancel();
	EndStage();
	tPhotoAsked = 0;
}

// Why not here (NULL: it can be had)
static const char *CantHere()
{
	if(Net::InitVars.mode != Net::MODE_SinglePlayer)
		return "single";
	player_t *p = Player();
	if(!p || p->state == player_t::PST_DEAD || p->health <= 0 || playstate != ex_stillplaying || automap != AMA_Off || gamestate.victoryflag)
		return "here";
	if(p->camera && p->camera != p->mo)
		return "here";
	return NULL;
}

static void Say(const char *why, int what)
{
	static const char *const names[] = { "", "Selfies", "Action Freeze", "The cinematic burst" };
	char msg[96];
	if(why && !strcmp(why, "single"))
		snprintf(msg, sizeof(msg), "%s %s for single player", names[what], what == STAGE_SELFIE ? "are" : "is");
	else if(what == STAGE_SELFIE)
		snprintf(msg, sizeof(msg), "Can't take a selfie here");
	else
		snprintf(msg, sizeof(msg), "Can't freeze here");
	IOSCamera_Message(msg);
}

extern "C" void IOSCamera_Photo(void)
{
	if(stage == STAGE_SELFIE || stage == STAGE_CINEMA)
		return;
	// in Action Freeze: a clean photo of the view
	const int kinds = stage == STAGE_FREEZE ? IOSCAPTURE_CLEAN : optKinds;
	if(IOSCapture_Request(kinds, IOSCAPTURE_MODE_PHOTO))
		tPhotoAsked = Now();
	else
		IOSCamera_Message("Still saving the last photo");
}

extern "C" void IOSCamera_Burst(void)
{
	if(stage != STAGE_NONE || burst)
		return;
	burst = true;
	ctBurst = 0;
	tBurstNext = Now();
	IOSPhotos_SetBatch(1);
}

static bool StartHeld(int which)
{
	EndBurst();
	if(stage != STAGE_NONE)
		return false;
	if(const char *why = CantHere())
	{
		Say(why, which);
		return false;
	}
	if(which != STAGE_FREEZE && !IOSCapture_HasRoom(IOSCAPTURE_CLEAN, 1))
	{
		IOSCamera_Message("Still saving the last photo");
		return false;
	}
	if(!ReadEye())
		return false;
	IOSCapture_Cancel(); // (a photo not taken yet would be of the held view)
#ifdef ECWOLF_IOSRIG
	CAMLOG("state hash before %08x\n", RigHash());
#endif
	randState ^= (unsigned)(Now() * 1000.0);
	if(!randState)
		randState = 2463534242u;
	stage = which; // (held from here: no tic runs)
	tStage = Now();
	ctTaken = 0;
	asked = false;
	tShown = tWaited = 0;
	cantWhy = NULL;
	const bool ok = which == STAGE_SELFIE ? SelfieBegin() : which == STAGE_FREEZE ? FreezeBegin() : CinemaBegin();
	if(!ok)
	{
		EndStage();
		if(cantWhy)
			IOSCamera_Message(cantWhy);
		else
			Say(NULL, which);
		return false;
	}
	if(which == STAGE_SELFIE)
	{
		ctShots = optShots;
		if(ctShots > 1)
			IOSPhotos_SetBatch(1);
	}
	if(which == STAGE_CINEMA)
	{
		ctShots = CINEMA_SHOTS;
		IOSPhotos_SetBatch(1);
		CinemaPlace(0);
	}
	if(which == STAGE_FREEZE)
		IOSPhotos_FreezeShown(1);
	return true;
}

extern "C" void IOSCamera_Selfie(void) { StartHeld(STAGE_SELFIE); }
extern "C" void IOSCamera_Cinematic(void) { StartHeld(STAGE_CINEMA); }

extern "C" void IOSCamera_Freeze(void)
{
	if(stage == STAGE_FREEZE)
	{
		IOSCamera_FreezeEnd();
		return;
	}
	if(!optFreeze)
	{
		IOSCamera_Message("Action Freeze is off (CAMERA's options)");
		return;
	}
	StartHeld(STAGE_FREEZE);
}

extern "C" int IOSCamera_IsBusy(void)
{
	return stage != STAGE_NONE || burst || IOSCapture_IsPending();
}

// A selfie's or the cinematic burst's photos, one a frame as they are taken
static void TickShots(double now)
{
	if(tShown > 0)
	{
		if(now - tShown > CAM_SHOW_TIME)
			EndStage();
		return;
	}
	if(asked)
	{
		if(IOSCapture_IsPending())
		{
			if(now - tWaited > CAM_GIVE_UP)
			{
				IOSCamera_Message("Couldn't take the photos");
				IOSCapture_Cancel();
				EndStage();
			}
			return;
		}
		asked = false;
		ctTaken++;
		if(ctTaken >= ctShots)
		{
			tShown = now;
			IOSPhotos_SetBatch(0);
			return;
		}
		if(stage == STAGE_SELFIE)
			Pose();
		else
			CinemaPlace(ctTaken);
		tWaited = 0;
	}
	if(!IOSCapture_Request(IOSCAPTURE_CLEAN, stage == STAGE_CINEMA ? IOSCAPTURE_MODE_BURST : IOSCAPTURE_MODE_SELFIE))
	{
		if(tWaited == 0)
			tWaited = now;
		if(now - tWaited > CAM_GIVE_UP)
		{
			IOSCamera_Message("Couldn't take the photos");
			EndStage();
		}
		return;
	}
	tWaited = now;
	asked = true;
}

void IOSCamera_Frame(bool bPlay)
{
	static bool init = false;
	if(!init)
	{
		init = true;
		IOSPhotos_Init();
	}
	IOSCapture_FrameBegin(screenWidth, screenHeight); // (the photos' size, for the budget)
	const double now = Now();
	if(!bPlay)
	{
		if(stage != STAGE_NONE || burst || IOSCapture_IsPending())
			IOSCamera_Abort();
		return;
	}
	if(stage != STAGE_NONE && CantHere())
	{
		IOSCapture_Cancel();
		EndStage();
	}
	// the people still there (a load or a level change ends it first; this
	// is in case)
	for(size_t i = 0;i < people.size();i++)
	{
		if(!StillThere(people[i].act))
		{
			IOSCapture_Cancel();
			EndStage();
			break;
		}
	}
	if(tPhotoAsked > 0 && !IOSCapture_IsPending())
		tPhotoAsked = 0;
	if(tPhotoAsked > 0 && now - tPhotoAsked > CAM_PHOTO_TIMEOUT && stage != STAGE_SELFIE && stage != STAGE_CINEMA)
	{
		IOSCapture_Cancel();
		tPhotoAsked = 0;
	}
	if(burst && now >= tBurstNext)
	{
		if(ctBurst >= CAM_BURST_MAX)
			EndBurst();
		else if(IOSCapture_Request(IOSCAPTURE_CLEAN, IOSCAPTURE_MODE_BURST))
		{
			ctBurst++;
			tBurstNext = now + CAM_BURST_GAP;
		}
	}
	if(stage == STAGE_SELFIE || stage == STAGE_CINEMA)
		TickShots(now);
	if(stage == STAGE_FREEZE)
		PlaceOrbit(false);
}

// ------------------------------------------------------------ the frames

static void Palette(unsigned char pal[768])
{
	for(int i = 0;i < 256;i++)
	{
		pal[i * 3 + 0] = GPalette.BaseColors[i].r;
		pal[i * 3 + 1] = GPalette.BaseColors[i].g;
		pal[i * 3 + 2] = GPalette.BaseColors[i].b;
	}
}

void IOSCamera_AfterScene(unsigned char *vbuf, unsigned pitch, int w, int h)
{
	if(stage == STAGE_SELFIE && camOn)
		DrawSelf(vbuf, pitch, w, h); // (BJ, holding the phone)
	if(!IOSCapture_IsPending())
		return;
	unsigned char pal[768];
	Palette(pal);
	IOSCapture_FrameBegin(screenWidth, screenHeight);
	IOSCapture_AfterScene(vbuf, (int)pitch, w, h, pal);
}

void IOSCamera_FrameDone()
{
	if(!IOSCapture_IsPending())
		return;
	unsigned char pal[768];
	Palette(pal);
	const BYTE *buf;
	int pitch;
	ESSType type;
	screen->GetScreenshotBuffer(buf, pitch, type);
	IOSCapture_FrameBegin(screen->GetWidth(), screen->GetHeight());
	if(type == SS_PAL)
		IOSCapture_AfterHud(buf, pitch, screen->GetWidth(), screen->GetHeight(), pal);
	screen->ReleaseScreenshotBuffer();
}

#ifdef ECWOLF_IOSRIG
// ------------------------------------------------------------ the rig

// Every actor's state, the map, the clock and every random number
// generator, as one number: the same before and after a selfie or a freeze
static unsigned Fnv(unsigned h, const void *p, size_t n)
{
	const unsigned char *b = (const unsigned char *)p;
	for(size_t i = 0;i < n;i++)
		h = (h ^ b[i]) * 16777619u;
	return h;
}

static unsigned RigHash()
{
	unsigned h = 2166136261u;
	int ct = 0;
	for(AActor::Iterator it = AActor::GetIterator();it.Next();)
	{
		AActor *a = it;
		h = Fnv(h, &a->x, sizeof(a->x));
		h = Fnv(h, &a->y, sizeof(a->y));
		h = Fnv(h, &a->z, sizeof(a->z));
		h = Fnv(h, &a->angle, sizeof(a->angle));
		h = Fnv(h, &a->pitch, sizeof(a->pitch));
		h = Fnv(h, &a->state, sizeof(a->state));
		h = Fnv(h, &a->sprite, sizeof(a->sprite));
		h = Fnv(h, &a->ticcount, sizeof(a->ticcount));
		h = Fnv(h, &a->health, sizeof(a->health));
		h = Fnv(h, &a->flags, sizeof(a->flags));
		h = Fnv(h, &a->velx, sizeof(a->velx));
		h = Fnv(h, &a->vely, sizeof(a->vely));
		h = Fnv(h, &a->dir, sizeof(a->dir));
		h = Fnv(h, &a->distance, sizeof(a->distance));
		h = Fnv(h, &a->viewx, sizeof(a->viewx));
		h = Fnv(h, &a->viewheight, sizeof(a->viewheight));
		h = Fnv(h, &a->transx, sizeof(a->transx));
		h = Fnv(h, &a->transy, sizeof(a->transy));
		ct++;
	}
	h = Fnv(h, &gamestate.TimeCount, sizeof(gamestate.TimeCount));
	if(player_t *p = Player())
	{
		h = Fnv(h, &p->FOV, sizeof(p->FOV));
		h = Fnv(h, &p->health, sizeof(p->health));
	}
	if(map)
	{
		const unsigned int n = map->GetHeader().width * map->GetHeader().height;
		for(unsigned int pl = 0;pl < map->NumPlanes();pl++)
			for(unsigned int i = 0;i < n;i++)
			{
				const MapSpot s = &map->GetPlane(pl).map[i];
				h = Fnv(h, &s->visible, sizeof(s->visible));
				h = Fnv(h, &s->amFlags, sizeof(s->amFlags));
			}
	}
	// the random number generators, as a save game writes them
	FILE *f = tmpfile();
	if(f)
	{
		FRandom::StaticWriteRNGState(f);
		rewind(f);
		unsigned char buf[4096];
		size_t got;
		while((got = fread(buf, 1, sizeof(buf), f)) > 0)
			h = Fnv(h, buf, got);
		fclose(f);
	}
	CAMLOG("(%d actors, tics %d, rng sum %08x, scale %d heightnumerator %d viewheight %d centerx %d)\n", ct, (int)gamestate.TimeCount,
		(unsigned)FRandom::StaticSumSeeds(), (int)scale, (int)heightnumerator, viewheight, (int)centerx);
	return h;
}

void IOSCamera_RigCommand(const char *cmd)
{
	float a = 0, b = 0;
	int n = 0;
	if(!strcmp(cmd, "photo")) IOSCamera_Photo();
	else if(!strcmp(cmd, "selfie")) IOSCamera_Selfie();
	else if(!strcmp(cmd, "burst")) IOSCamera_Burst();
	else if(!strcmp(cmd, "freeze")) IOSCamera_Freeze();
	else if(!strcmp(cmd, "cinema")) IOSCamera_Cinematic();
	else if(!strcmp(cmd, "end")) IOSCamera_FreezeEnd();
	else if(!strcmp(cmd, "options")) IOSCamera_ShowOptions();
	else if(sscanf(cmd, "orbit %f %f", &a, &b) == 2) IOSCamera_FreezeOrbit(a, b);
	else if(sscanf(cmd, "pan %f %f", &a, &b) == 2) IOSCamera_FreezePan(a, b);
	else if(sscanf(cmd, "zoom %f", &a) == 1) IOSCamera_FreezeZoom(a);
	else if(sscanf(cmd, "kinds %d", &n) == 1) IOSCamera_SetKinds(n);
	else if(sscanf(cmd, "selfies %d", &n) == 1) IOSCamera_SetSelfieShots(n);
	else if(sscanf(cmd, "freezeon %d", &n) == 1) IOSCamera_SetFreezeOn(n);
	else if(!strcmp(cmd, "hash")) CAMLOG("state hash %08x\n", RigHash());
	else if(!strcmp(cmd, "status"))
		CAMLOG("stage %d cam %d figure %d people %d hidden %d taken %d busy %d eye %.2f %.2f h %.2f yaw %.0f tilt %.2f zoom %.2f\n",
			stage, (int)camOn, (int)showFigure, (int)people.size(), (int)hidden.size(), ctTaken, IOSCamera_IsBusy(), cam.eye.x, cam.eye.y,
			cam.h, cam.yaw * 180 / CAM_PI, cam.tilt, cam.zoom);
	else if(sscanf(cmd, "goto %f %f %d", &a, &b, &n) == 3)
	{
		// (the rig only: the player put at tile x y facing angle n degrees)
		player_t *p = Player();
		if(!p)
			return;
		// (the nearest open tile)
		for(int r = 0, found = 0;r < 4 && !found;r++)
			for(int dy = -r;dy <= r && !found;dy++)
				for(int dx = -r;dx <= r && !found;dx++)
					if(!Solid((int)a + dx, (int)b + dy) && map->GetSpot((int)a + dx, (int)b + dy, 0)->zone)
					{
						a = (int)a + dx + 0.5f;
						b = (int)b + dy + 0.5f;
						found = 1;
					}
		p->mo->x = (fixed)lround(a * 65536.0);
		p->mo->y = (fixed)lround(b * 65536.0);
		p->mo->angle = ToAngle(n * CAM_PI / 180.0);
		p->mo->EnterZone(map->GetSpot((int)a, (int)b, 0)->zone);
		const MapSpot spot = map->GetSpot((int)a, (int)b, 0);
		CAMLOG("went to %.1f %.1f %d (%s, zone %d)\n", a, b, n, Solid((int)a, (int)b) ? "solid" : "open", spot->zone ? (int)spot->zone->index : -1);
	}
	else if(!strcmp(cmd, "groups"))
	{
		// (the rig only: the areas with the most living enemies)
		std::vector<int> ct(4096, 0);
		std::vector<V2> at(4096);
		for(AActor::Iterator it = AActor::GetIterator();it.Next();)
		{
			AActor *e = it;
			if(!Alive(e) || e == Player()->mo || !e->GetZone() || e->GetZone()->index >= 4096)
				continue;
			ct[e->GetZone()->index]++;
			at[e->GetZone()->index] = PosOf(e);
		}
		for(int z = 0;z < 4096;z++)
			if(ct[z] >= 3)
				CAMLOG("group: zone %d, %d enemies, one at %.1f %.1f\n", z, ct[z], at[z].x, at[z].y);
	}
	else if(!strcmp(cmd, "who"))
	{
		// (the rig only: the living enemies, how far, whether a selfie takes them)
		if(!ReadEye())
			return;
		const MapZone *mine = Player()->mo->GetZone();
		for(AActor::Iterator it = AActor::GetIterator();it.Next();)
		{
			AActor *e = it;
			if(!Alive(e) || e == Player()->mo)
				continue;
			const double d = Len(PosOf(e) - eyeAt);
			if(d > 30)
				continue;
			const MapZone *z = e->GetZone();
			CAMLOG("who %s at %.1f %.1f: %.1f away, zone %d (mine %d)%s%s%s\n", e->GetClass()->GetName().GetChars(), PosOf(e).x, PosOf(e).y, d,
				z ? (int)z->index : -1, mine ? (int)mine->index : -1,
				(mine && z && (mine == z || map->CheckLink(mine, z, true))) ? " linked" : "", ClearLine(eyeAt, PosOf(e)) ? " seen" : "",
				HereWithPlayer(e, d) ? " HERE" : "");
		}
	}
	else if(sscanf(cmd, "gather %d", &n) == 1)
	{
		// (the rig only: the nearest n enemies brought to stand ahead, for
		// selfies where none are near; changes the game, unlike the rest)
		if(!ReadEye())
			return;
		FindPeople(1000.0, n);
		const double room = FreeAhead(eyeAt, eyeYaw, 6.0);
		for(size_t i = 0;i < people.size();i++)
		{
			const V2 p = eyeAt + Fwd(eyeYaw) * fmin(room - 0.5, 2.0 + 0.6 * (i / 3)) + Right(eyeYaw) * (((int)(i % 3) - 1) * 0.7);
			people[i].act->x = (fixed)lround(p.x * 65536);
			people[i].act->y = (fixed)lround(p.y * 65536);
		}
		CAMLOG("gathered %d\n", (int)people.size());
		people.clear();
	}
	else
		CAMLOG("unknown command '%s'\n", cmd);
}
#endif
