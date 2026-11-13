// The touch HUD (MENU's tray's HUD: IOSTouch.h's IOSTOUCH_HUD_*), ECWolf's
// side: SPLIT and STRIP draw dark rounded pills of the game's own status bar
// graphics over the 3D view (BJ's face, the first-aid kit and health, the
// extra life and lives; the gun in hand in light grey, its ammo, the keys),
// where the touch controls say (IOSTouch_GetHudLayout: the owner's approved
// mockups, the UZDoom port's sizes), so the HUD and FIRE and USE agree.
// CLASSIC is ECWolf's own status bar, faded by the HUD slider
// (IOSHud_FadeStatusBar). Everything is made at run time from the
// player's own data; nothing of it is in the repository.
//
// ECWolf draws in 8 bits: the pills are composed in RGB over the frame and
// put back as the palette's nearest colours.

#include <cmath>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <SDL.h>

#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "id_ca.h"
#include "v_video.h"
#include "v_palette.h"
#include "v_font.h"
#include "colormatcher.h"
#include "textures/textures.h"
#include "a_inventory.h"
#include "a_keys.h"
#include "g_mapinfo.h"
#include "thingdef/thingdef.h"
#include "IOSTouch.h"
#include "IOSCamera.h"
#include "IOSHud.h"

extern "C" int IOSGame_LastPictureRect(int r[6]);

namespace {

// A picture: RGBA, not premultiplied (a = 0: nothing there)
struct Pic
{
	int w, h;
	std::vector<unsigned char> px; // w * h * 4
	Pic() : w(0), h(0) {}
	Pic(int iw, int ih) : w(iw), h(ih), px((size_t)(iw > 0 ? iw : 0) * (ih > 0 ? ih : 0) * 4, 0) {}
	bool Ok() const { return w > 0 && h > 0; }
	unsigned char *At(int x, int y) { return &px[((size_t)y * w + x) * 4]; }
	const unsigned char *At(int x, int y) const { return &px[((size_t)y * w + x) * 4]; }
};

// A texture's pixels in the base palette's colours
Pic FromTexture(FTexture *tex)
{
	if(!tex || tex->GetWidth() <= 0 || tex->GetHeight() <= 0)
		return Pic();
	Pic p(tex->GetWidth(), tex->GetHeight());
	for(int x = 0;x < p.w;x++)
	{
		const FTexture::Span *sp;
		const BYTE *col = tex->GetColumn(x, &sp);
		for(;sp && sp->Length;sp++)
			for(int y = sp->TopOffset;y < sp->TopOffset + sp->Length && y < p.h;y++)
			{
				const PalEntry &c = GPalette.BaseColors[col[y]];
				unsigned char *d = p.At(x, y);
				d[0] = c.r;
				d[1] = c.g;
				d[2] = c.b;
				d[3] = 255;
			}
	}
	return p;
}

Pic FromName(const char *name, int use = FTexture::TEX_Any)
{
	const FTextureID id = TexMan.CheckForTexture(name, use);
	return id.isValid() ? FromTexture(TexMan[id]) : Pic();
}

// The opaque part's bounding box
Pic CropAlpha(const Pic &p)
{
	int x0 = p.w, y0 = p.h, x1 = -1, y1 = -1;
	for(int y = 0;y < p.h;y++)
		for(int x = 0;x < p.w;x++)
			if(p.At(x, y)[3])
			{
				if(x < x0) x0 = x;
				if(x > x1) x1 = x;
				if(y < y0) y0 = y;
				if(y > y1) y1 = y;
			}
	if(x1 < 0)
		return Pic();
	Pic o(x1 - x0 + 1, y1 - y0 + 1);
	for(int y = 0;y < o.h;y++)
		for(int x = 0;x < o.w;x++)
			memcpy(o.At(x, y), p.At(x + x0, y + y0), 4);
	return o;
}

Pic Crop(const Pic &p, int x0, int y0, int x1, int y1)
{
	if(x1 > p.w) x1 = p.w;
	if(y1 > p.h) y1 = p.h;
	if(x0 < 0) x0 = 0;
	if(y0 < 0) y0 = 0;
	if(x1 <= x0 || y1 <= y0)
		return Pic();
	Pic o(x1 - x0, y1 - y0);
	for(int y = 0;y < o.h;y++)
		for(int x = 0;x < o.w;x++)
			memcpy(o.At(x, y), p.At(x + x0, y + y0), 4);
	return o;
}

// The status bar's blue (its boxes' fill and bevels) keyed out
void Unblue(Pic &p)
{
	for(int y = 0;y < p.h;y++)
		for(int x = 0;x < p.w;x++)
		{
			unsigned char *c = p.At(x, y);
			if(c[3] && c[2] > c[0] + 60 && c[2] > c[1] + 60)
				c[3] = 0;
		}
}

// The background (the colours most of the border is, and anything near
// them) keyed out from the edges in: the face's panel
void KeyOut(Pic &p)
{
	const int thr = 48;
	std::map<unsigned, int> count;
	int border = 0;
	for(int y = 0;y < p.h;y++)
		for(int x = 0;x < p.w;x++)
			if(x == 0 || y == 0 || x == p.w - 1 || y == p.h - 1)
			{
				const unsigned char *c = p.At(x, y);
				count[(c[0] << 16) | (c[1] << 8) | c[2]]++;
				border++;
			}
	std::vector<unsigned> bg;
	for(std::map<unsigned, int>::const_iterator i = count.begin();i != count.end();++i)
		if(i->second * 20 >= border)
			bg.push_back(i->first);
	std::vector<bool> seen((size_t)p.w * p.h, false);
	std::vector<int> todo;
	for(int y = 0;y < p.h;y++)
		for(int x = 0;x < p.w;x++)
			if(x == 0 || y == 0 || x == p.w - 1 || y == p.h - 1)
				todo.push_back(y * p.w + x);
	while(!todo.empty())
	{
		const int i = todo.back();
		todo.pop_back();
		if(seen[i])
			continue;
		seen[i] = true;
		const int x = i % p.w, y = i / p.w;
		unsigned char *c = p.At(x, y);
		if(!c[3])
			continue;
		bool near = false;
		for(size_t b = 0;b < bg.size() && !near;b++)
		{
			const int dr = c[0] - (int)(bg[b] >> 16), dg = c[1] - (int)((bg[b] >> 8) & 255), db = c[2] - (int)(bg[b] & 255);
			near = dr * dr + dg * dg + db * db < thr * thr;
		}
		if(!near)
			continue;
		c[3] = 0;
		if(x + 1 < p.w) todo.push_back(i + 1);
		if(x > 0) todo.push_back(i - 1);
		if(y + 1 < p.h) todo.push_back(i + p.w);
		if(y > 0) todo.push_back(i - p.w);
	}
}

// Scaled to w x h, nearest (as PIL's)
Pic Resize(const Pic &p, int w, int h)
{
	if(!p.Ok() || w <= 0 || h <= 0)
		return Pic();
	Pic o(w, h);
	for(int y = 0;y < h;y++)
	{
		int sy = (int)((y + 0.5) * p.h / h);
		if(sy >= p.h) sy = p.h - 1;
		for(int x = 0;x < w;x++)
		{
			int sx = (int)((x + 0.5) * p.w / w);
			if(sx >= p.w) sx = p.w - 1;
			memcpy(o.At(x, y), p.At(sx, sy), 4);
		}
	}
	return o;
}

Pic Scale(const Pic &p, double k)
{
	if(!p.Ok())
		return Pic();
	const int w = (int)lround(p.w * k), h = (int)lround(p.h * k);
	return Resize(p, w < 1 ? 1 : w, h < 1 ? 1 : h);
}

// The gun's picture lit light grey (its shading kept faintly)
void LightGrey(Pic &p)
{
	for(size_t i = 0;i < p.px.size();i += 4)
		if(p.px[i + 3])
		{
			const double l = (p.px[i] + p.px[i + 1] + p.px[i + 2]) / 3.0;
			const int v = (int)(205 + l * 0.2);
			p.px[i] = p.px[i + 1] = (unsigned char)v;
			p.px[i + 2] = (unsigned char)(v + 6 > 255 ? 255 : v + 6);
		}
}

// Red: the warnings (health under 25, ammo low)
void TintRed(Pic &p)
{
	for(size_t i = 0;i < p.px.size();i += 4)
		if(p.px[i + 3])
		{
			const double l = (p.px[i] + p.px[i + 1] + p.px[i + 2]) / 765.0;
			const int r = (int)(255 * (0.55 + 0.45 * l));
			p.px[i] = (unsigned char)(r > 255 ? 255 : r);
			p.px[i + 1] = (unsigned char)(40 * l);
			p.px[i + 2] = (unsigned char)(30 * l);
		}
}

void Paste(Pic &dst, const Pic &src, int x0, int y0)
{
	for(int y = 0;y < src.h;y++)
		for(int x = 0;x < src.w;x++)
		{
			const unsigned char *s = src.At(x, y);
			if(s[3] && x + x0 >= 0 && y + y0 >= 0 && x + x0 < dst.w && y + y0 < dst.h)
				memcpy(dst.At(x + x0, y + y0), s, 4);
		}
}

// ------------------------------------------------------------ the pictures

struct Gfx
{
	bool loaded;
	Pic digit[10], pct, kit, oneUp;
	std::map<int, Pic> faces;   // by texture: keyed out, cropped
	std::map<int, Pic> guns;    // by texture: unblued, cropped, light grey
	std::map<int, Pic> keys;    // 1..4
	Gfx() : loaded(false) {}
};
Gfx gfx;

void LoadGfx()
{
	if(gfx.loaded)
		return;
	gfx.loaded = true;
	// the status bar's digits (its font's pictures, FONTN048...)
	for(int i = 0;i < 10;i++)
	{
		FString name;
		name.Format("FONTN%03d", 48 + i);
		Pic p = FromName(name);
		Unblue(p);
		gfx.digit[i] = CropAlpha(p);
	}
	// the status bar's own %, from its background picture (by HEALTH)
	{
		Pic st = FromName("STBACK");
		if(st.Ok())
		{
			Pic r = Crop(st, 192, 18, 201, 32);
			Unblue(r);
			gfx.pct = CropAlpha(r);
		}
	}
	gfx.kit = CropAlpha(FromName("MEDIA0", FTexture::TEX_Sprite));
	{
		Pic o = CropAlpha(FromName("ONUPA0", FTexture::TEX_Sprite));
		if(o.Ok())
			o = CropAlpha(Crop(o, 0, 0, o.w, o.h - 4));
		gfx.oneUp = o;
	}
	for(int k = 1;k <= 4;k++)
	{
		FString name;
		name.Format("STKEYS%d", k);
		Pic p = FromName(name);
		Unblue(p);
		gfx.keys[k] = CropAlpha(p);
	}
	printf("iOS: HUD pictures: digits %d, %% %d, kit %d, extra life %d\n", gfx.digit[0].Ok(), gfx.pct.Ok(), gfx.kit.Ok(), gfx.oneUp.Ok());
}

const Pic &FacePic(FTextureID id)
{
	std::map<int, Pic>::iterator i = gfx.faces.find(id.GetIndex());
	if(i != gfx.faces.end())
		return i->second;
	Pic p = FromTexture(TexMan[id]);
	KeyOut(p);
	return gfx.faces[id.GetIndex()] = CropAlpha(p);
}

const Pic &GunPic(FTextureID id)
{
	std::map<int, Pic>::iterator i = gfx.guns.find(id.GetIndex());
	if(i != gfx.guns.end())
		return i->second;
	Pic p = FromTexture(TexMan[id]);
	Unblue(p);
	p = CropAlpha(p);
	LightGrey(p);
	return gfx.guns[id.GetIndex()] = p;
}

// A number in the status bar's digits, twice their size, 4 apart
Pic Number(int n, bool red)
{
	char str[16];
	snprintf(str, sizeof(str), "%d", n < 0 ? 0 : n);
	std::vector<Pic> d;
	int w = 0, h = 0;
	for(const char *c = str;*c;c++)
	{
		d.push_back(Scale(gfx.digit[*c - '0'], 2));
		w += d.back().w;
		if(d.back().h > h) h = d.back().h;
	}
	if(d.empty() || h <= 0)
		return Pic();
	w += 4 * ((int)d.size() - 1);
	Pic o(w, h);
	int x = 0;
	for(size_t i = 0;i < d.size();i++)
	{
		Paste(o, d[i], x, h - d[i].h);
		x += d[i].w + 4;
	}
	if(red)
		TintRed(o);
	return o;
}

// ------------------------------------------------------------ the pill

struct Part
{
	bool gap;
	Pic pic;
	bool glow;
	Part() : gap(true), glow(false) {}
	Part(const Pic &p, bool g = false) : gap(false), pic(p), glow(g) {}
};
typedef std::vector<Part> Parts;

double PartW(const Part &p) { return p.gap ? 22 : p.pic.w; }
double PillW(const Parts &ps)
{
	double w = 44;
	for(size_t i = 0;i < ps.size();i++)
		w += PartW(ps[i]);
	return w + 8 * (ps.size() > 1 ? ps.size() - 1 : 0);
}

// The palette's nearest colour, by 5 bits a channel
BYTE nearest[32768];
bool nearestMade = false;
void MakeNearest()
{
	if(nearestMade)
		return;
	nearestMade = true;
	for(int r = 0;r < 32;r++)
		for(int g = 0;g < 32;g++)
			for(int b = 0;b < 32;b++)
				nearest[(r << 10) | (g << 5) | b] = ColorMatcher.Pick((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
}

// The frame (8 bits) and an RGB copy of the part being drawn
struct Canvas
{
	BYTE *buf;
	int pitch, w, h;
	int x0, y0, cw, ch;        // the RGB copy's place
	std::vector<float> rgb;

	void Begin(int ix0, int iy0, int ix1, int iy1)
	{
		if(ix0 < 0) ix0 = 0;
		if(iy0 < 0) iy0 = 0;
		if(ix1 > w) ix1 = w;
		if(iy1 > h) iy1 = h;
		x0 = ix0;
		y0 = iy0;
		cw = ix1 > ix0 ? ix1 - ix0 : 0;
		ch = iy1 > iy0 ? iy1 - iy0 : 0;
		rgb.assign((size_t)cw * ch * 3, 0.0f);
		for(int y = 0;y < ch;y++)
			for(int x = 0;x < cw;x++)
			{
				const PalEntry &c = GPalette.BaseColors[buf[(y + y0) * pitch + x + x0]];
				float *d = &rgb[((size_t)y * cw + x) * 3];
				d[0] = c.r;
				d[1] = c.g;
				d[2] = c.b;
			}
	}
	void End()
	{
		for(int y = 0;y < ch;y++)
			for(int x = 0;x < cw;x++)
			{
				const float *s = &rgb[((size_t)y * cw + x) * 3];
				int c[3];
				for(int k = 0;k < 3;k++)
				{
					c[k] = (int)(s[k] + 0.5f);
					if(c[k] < 0) c[k] = 0;
					if(c[k] > 255) c[k] = 255;
				}
				buf[(y + y0) * pitch + x + x0] = nearest[((c[0] >> 3) << 10) | ((c[1] >> 3) << 5) | (c[2] >> 3)];
			}
	}
	void Blend(int x, int y, float r, float g, float b, float a)
	{
		x -= x0;
		y -= y0;
		if(a <= 0 || x < 0 || y < 0 || x >= cw || y >= ch)
			return;
		if(a > 1) a = 1;
		float *d = &rgb[((size_t)y * cw + x) * 3];
		d[0] += (r - d[0]) * a;
		d[1] += (g - d[1]) * a;
		d[2] += (b - d[2]) * a;
	}
};

// How much of pixel (px, py) a rounded rectangle covers (4 x 4 samples)
float RoundRectCover(double px, double py, double x0, double y0, double x1, double y1, double r)
{
	int in = 0;
	for(int sy = 0;sy < 4;sy++)
		for(int sx = 0;sx < 4;sx++)
		{
			const double x = px + (sx + 0.5) / 4, y = py + (sy + 0.5) / 4;
			if(x < x0 || x > x1 || y < y0 || y > y1)
				continue;
			const double cx = x < x0 + r ? x0 + r : (x > x1 - r ? x1 - r : x);
			const double cy = y < y0 + r ? y0 + r : (y > y1 - r ? y1 - r : y);
			if((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r)
				in++;
		}
	return in / 16.0f;
}

// A red glow round a picture (its shape blurred), at (px, py)
void Glow(Canvas &cv, const Pic &im, int px, int py, double u, float a)
{
	const double sigma = 7 * u;
	const int pad = (int)ceil(sigma * 3);
	const int w = im.w + 2 * pad, h = im.h + 2 * pad;
	std::vector<float> m((size_t)w * h, 0.0f), t((size_t)w * h, 0.0f);
	for(int y = 0;y < im.h;y++)
		for(int x = 0;x < im.w;x++)
			m[(size_t)(y + pad) * w + x + pad] = im.At(x, y)[3] / 255.0f;
	// three box blurs, about a gaussian
	const int rad = (int)lround(sqrt(12 * sigma * sigma / 3 + 1) / 2);
	for(int pass = 0;pass < 3 && rad > 0;pass++)
	{
		for(int y = 0;y < h;y++)
			for(int x = 0;x < w;x++)
			{
				float s = 0;
				for(int k = -rad;k <= rad;k++)
				{
					const int xx = x + k;
					if(xx >= 0 && xx < w) s += m[(size_t)y * w + xx];
				}
				t[(size_t)y * w + x] = s / (2 * rad + 1);
			}
		for(int y = 0;y < h;y++)
			for(int x = 0;x < w;x++)
			{
				float s = 0;
				for(int k = -rad;k <= rad;k++)
				{
					const int yy = y + k;
					if(yy >= 0 && yy < h) s += t[(size_t)yy * w + x];
				}
				m[(size_t)y * w + x] = s / (2 * rad + 1);
			}
	}
	for(int y = 0;y < h;y++)
		for(int x = 0;x < w;x++)
		{
			// (as the mockups' blur of red on nothing: darker as it thins,
			// twice as strong)
			const float g = m[(size_t)y * w + x], g2 = g * 2;
			cv.Blend(px - pad + x, py - pad + y, 255 * g, 30 * g, 20 * g, (g2 > 1 ? 1 : g2) * a);
		}
}

// One pill: its left x0 and bottom y1 in the frame's pixels, u frame
// pixels a mockup pixel, alpha the HUD slider's
void DrawPill(Canvas &cv, const Parts &parts, double x0, double y1, double u, float alpha, float pulse)
{
	const double w = PillW(parts) * u, h = 64 * u, y0 = y1 - h;
	const int ix0 = (int)floor(x0), iy0 = (int)floor(y0);
	const int ix1 = (int)ceil(x0 + w) + 1, iy1 = (int)ceil(y1) + 1;
	// (the red glow reaches outside the pill)
	const int pad = (int)ceil(7 * u * 3) + 2;
	cv.Begin(ix0 - pad, iy0 - pad, ix1 + pad, iy1 + pad);
	const double r = 26 * u, e = 2 * u;
	for(int y = iy0;y < iy1;y++)
		for(int x = ix0;x < ix1;x++)
		{
			const float co = RoundRectCover(x, y, x0, y0, x0 + w, y0 + h, r);
			if(co <= 0)
				continue;
			cv.Blend(x, y, 255, 255, 255, co * 55 / 255.0f * alpha);
			const float ci = RoundRectCover(x, y, x0 + e, y0 + e, x0 + w - e, y0 + h - e, r - e);
			cv.Blend(x, y, 10, 6, 4, ci * 135 / 255.0f * alpha);
		}
	// the parts, from 22 in, 8 apart, centred on the pill's middle
	const int cx0 = (int)lround(x0), cy0 = (int)lround(y0);
	const double ym = h / 2;
	double x = 22 * u;
	for(size_t i = 0;i < parts.size();i++)
	{
		const Part &p = parts[i];
		if(p.gap)
		{
			// a thin line: x + 10 to x + 12, 14 either side of the middle
			const double lx0 = cx0 + x + 10 * u, lx1 = cx0 + x + 12 * u + 1;
			const double ly0 = cy0 + ym - 14 * u, ly1 = cy0 + ym + 14 * u + 1;
			for(int py = (int)floor(ly0);py < (int)ceil(ly1);py++)
				for(int px = (int)floor(lx0);px < (int)ceil(lx1);px++)
				{
					const double cxv = std::min<double>(px + 1, lx1) - std::max<double>(px, lx0);
					const double cyv = std::min<double>(py + 1, ly1) - std::max<double>(py, ly0);
					if(cxv > 0 && cyv > 0)
						cv.Blend(px, py, 255, 255, 255, (float)(cxv * cyv) * 60 / 255.0f * alpha);
				}
		}
		else if(p.pic.Ok())
		{
			Pic im = u != 1 ? Resize(p.pic, std::max(1, (int)lround(p.pic.w * u)), std::max(1, (int)lround(p.pic.h * u))) : p.pic;
			const int px = cx0 + (int)lround(x), py = cy0 + (int)lround(ym - im.h / 2.0);
			if(p.glow)
				Glow(cv, im, px, py, u, alpha * pulse);
			for(int yy = 0;yy < im.h;yy++)
				for(int xx = 0;xx < im.w;xx++)
				{
					const unsigned char *c = im.At(xx, yy);
					if(c[3])
						cv.Blend(px + xx, py + yy, c[0], c[1], c[2], c[3] / 255.0f * alpha);
				}
		}
		x += PartW(p) * u + 8 * u;
	}
	cv.End();
}

// What the pills show now
void MakeParts(player_t &pl, bool face, Parts &you, Parts &gun)
{
	if(face)
	{
		FTextureID id = gamestate.faceframe;
		if(pl.health <= 0)
		{
			static const ClassDef *schabbs = ClassDef::FindClass("Schabbs");
			id = TexMan.CheckForTexture(pl.killerobj && schabbs && pl.killerobj->IsKindOf(schabbs) ? "STFMUT0" : "STFDEAD0", FTexture::TEX_Any);
		}
		if(!id.isValid())
			id = TexMan.CheckForTexture("STFST00", FTexture::TEX_Any);
		if(id.isValid())
		{
			const Pic &f = FacePic(id);
			if(f.Ok())
			{
				// as 32 pixels high: 50
				you.push_back(Part(Scale(f, 50.0 / 32.0)));
				you.push_back(Part());
			}
		}
	}
	const bool low = pl.health < 25;
	{
		Pic hp = Number(pl.health, low);
		Pic pct = Scale(gfx.pct, 2);
		if(low)
			TintRed(pct);
		Pic hpw(hp.w + (pct.Ok() ? 4 + pct.w : 0), hp.h);
		Paste(hpw, hp, 0, 0);
		if(pct.Ok())
			Paste(hpw, pct, hp.w + 4, hp.h - pct.h);
		if(gfx.kit.Ok())
			you.push_back(Part(Scale(gfx.kit, 1.5)));
		you.push_back(Part(hpw, low));
	}
	if(gamestate.difficulty && gamestate.difficulty->LivesCount >= 0)
	{
		you.push_back(Part());
		if(gfx.oneUp.Ok())
			you.push_back(Part(Scale(gfx.oneUp, 30.0 / gfx.oneUp.h)));
		you.push_back(Part(Number(pl.lives, false)));
	}
	if(AWeapon *wpn = pl.ReadyWeapon)
	{
		if(!wpn->icon.isNull())
		{
			const Pic &g = GunPic(wpn->icon);
			if(g.Ok())
				gun.push_back(Part(Scale(g, 1.25)));
		}
		if(AAmmo *ammo = wpn->ammo[AWeapon::PrimaryFire])
		{
			const int n = ammo->amount;
			gun.push_back(Part(Number(n, n <= 0 || n < ammo->maxamount * 0.25)));
		}
	}
	// the keys held, as the status bar's (STKEYS1 gold, STKEYS2 silver...)
	Parts keys;
	if(pl.mo)
	{
		unsigned int have = 0;
		for(AInventory *item = pl.mo->inventory;item;item = item->inventory)
			if(item->IsKindOf(NATIVE_CLASS(Key)))
			{
				const unsigned int slot = static_cast<AKey *>(item)->KeyNumber;
				if(slot >= 1 && slot <= 4)
					have |= 1u << (slot - 1);
			}
		for(int k = 1;k <= 4;k++)
			if((have & (1u << (k - 1))) && gfx.keys[k].Ok())
				keys.push_back(Part(Scale(gfx.keys[k], 2)));
	}
	if(!keys.empty())
	{
		if(!gun.empty())
			gun.push_back(Part());
		gun.insert(gun.end(), keys.begin(), keys.end());
	}
}

// Where the game's picture is on the screen, to put a fraction of the
// screen into the frame's pixels: frame = (f * out - pic) * k
struct Map
{
	double ox, oy, kx, ky, outW, outH;
	void Make(int fw, int fh)
	{
		int r[6];
		if(IOSGame_LastPictureRect(r) && r[2] > 0 && r[3] > 0 && r[4] > 0 && r[5] > 0)
		{
			ox = r[0];
			oy = r[1];
			kx = (double)fw / r[2];
			ky = (double)fh / r[3];
			outW = r[4];
			outH = r[5];
		}
		else
		{
			ox = oy = 0;
			kx = ky = 1;
			outW = fw;
			outH = fh;
		}
	}
	double X(double f) const { return (f * outW - ox) * kx; }
	double Y(double f) const { return (f * outH - oy) * ky; }
};

} // namespace

void IOSHud_Draw()
{
	IOSTouchHudLayout hl;
	IOSTouch_GetHudLayout(&hl);
	if(hl.iStyle != IOSTOUCH_HUD_SPLIT && hl.iStyle != IOSTOUCH_HUD_STRIP)
		return;
	if(viewsize < 21 || !ingame || IOSCamera_HidesWeapon() || gamestate.victoryflag)
		return;
	player_t &pl = players[ConsolePlayer];
	if(!pl.mo || hl.fPillH <= 0)
		return;
	BYTE *buf = screen->GetBuffer();
	if(!buf)
		return;
	LoadGfx();
	MakeNearest();
	Canvas cv;
	cv.buf = buf;
	cv.pitch = screen->GetPitch();
	cv.w = screen->GetWidth();
	cv.h = screen->GetHeight();
	Map m;
	m.Make(cv.w, cv.h);

	Parts you, gun;
	MakeParts(pl, hl.bFace != 0, you, gun);
	const float alpha = hl.fAlpha < 0.2f ? 0.2f : (hl.fAlpha > 1 ? 1 : hl.fAlpha);
	// the low-health glow pulses twice a second
	const float pulse = 0.35f + 0.65f * (0.5f + 0.5f * (float)sin(SDL_GetTicks() * (2 * M_PI * 2 / 1000.0)));
	const double bottom = m.Y(hl.fPillBottom);
	double u = (m.Y(hl.fPillBottom) - m.Y(hl.fPillBottom - hl.fPillH)) / 64;
	if(hl.iStyle == IOSTOUCH_HUD_SPLIT)
	{
		const double left = m.X(hl.fLeft), right = m.X(hl.fRight);
		const double room = ((right - left) / 2 - 12 * u) / u;
		const double need = std::max(PillW(you), gun.empty() ? 0.0 : PillW(gun));
		if(need > room)
			u *= room / need;
		DrawPill(cv, you, left, bottom, u, alpha, pulse);
		if(!gun.empty())
			DrawPill(cv, gun, right - PillW(gun) * u, bottom, u, alpha, pulse);
	}
	else
	{
		Parts all = you;
		if(!gun.empty())
		{
			all.push_back(Part());
			all.insert(all.end(), gun.begin(), gun.end());
		}
		const double mid = m.X(0.5), half = m.X(0.5 + hl.fStripHalf) - mid;
		const double room = 2 * half / u, need = PillW(all);
		if(need > room)
			u *= room / need;
		DrawPill(cv, all, mid - PillW(all) * u / 2, bottom, u, alpha, pulse);
	}
}

// CLASSIC's status bar faded by the HUD slider: over the view's border (the
// view doesn't reach under it). Before the bar is drawn, its place is filled
// with the border and kept; after, the bar is mixed with it.
static std::vector<BYTE> fadeUnder;
static int fadeRect[4];
static float fadeAlpha = 1;

bool IOSHud_StatusBarFadeBegin()
{
	IOSTouchHudLayout hl;
	IOSTouch_GetHudLayout(&hl);
	fadeAlpha = hl.fAlpha;
	if(hl.iStyle != IOSTOUCH_HUD_CLASSIC || fadeAlpha >= 0.999f || viewsize >= 21 || !ingame || !levelInfo)
		return false;
	const int x0 = statusbarx, y0 = statusbary2, x1 = (int)screenWidth - statusbarx, y1 = (int)screenHeight;
	if(x1 <= x0 || y1 <= y0 || y0 < 0)
		return false;
	VWB_DrawFill(TexMan(levelInfo->GetBorderTexture()), x0, y0, x1, y1);
	BYTE *buf = VL_LockSurface();
	if(!buf)
		return false;
	const int pitch = SCREENPITCH, w = SCREENWIDTH, h = SCREENHEIGHT;
	fadeRect[0] = x0;
	fadeRect[1] = y0;
	fadeRect[2] = x1 < w ? x1 : w;
	fadeRect[3] = y1 < h ? y1 : h;
	fadeUnder.resize((size_t)(fadeRect[2] - x0) * (fadeRect[3] - y0));
	for(int y = y0;y < fadeRect[3];y++)
		memcpy(&fadeUnder[(size_t)(y - y0) * (fadeRect[2] - x0)], buf + y * pitch + x0, fadeRect[2] - x0);
	VL_UnlockSurface();
	return true;
}

void IOSHud_StatusBarFadeEnd()
{
	BYTE *buf = VL_LockSurface();
	if(!buf)
		return;
	MakeNearest();
	const int pitch = SCREENPITCH;
	const int x0 = fadeRect[0], y0 = fadeRect[1], x1 = fadeRect[2], y1 = fadeRect[3];
	const float a = fadeAlpha < 0.2f ? 0.2f : fadeAlpha;
	for(int y = y0;y < y1;y++)
		for(int x = x0;x < x1;x++)
		{
			BYTE &d = buf[y * pitch + x];
			const PalEntry &top = GPalette.BaseColors[d];
			const PalEntry &under = GPalette.BaseColors[fadeUnder[(size_t)(y - y0) * (x1 - x0) + x - x0]];
			const int r = (int)(under.r + (top.r - under.r) * a + 0.5f);
			const int g = (int)(under.g + (top.g - under.g) * a + 0.5f);
			const int b = (int)(under.b + (top.b - under.b) * a + 0.5f);
			d = nearest[((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)];
		}
	VL_UnlockSurface();
}
