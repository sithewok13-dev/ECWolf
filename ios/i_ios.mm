/*
** i_ios.mm: the iOS app around ECWolf (Objective-C++ with ARC); from the
** owner's UZDoom and Dark Forces ports (ios/i_ios.mm, ios/IOSGameData.m)
**
** - main (SDL_main): SDL's hints, the sound session, the first screen until
**   the player's game files are in the app's Documents (Files > On My
**   iPhone > Wolf3D), then the game
** - FileSys::OSX_FindFolder: where ECWolf keeps its settings, saves and
**   screenshots (Documents/ECWolf: they show in Files and are kept)
** - I_PickIWad_Cocoa: the game picker (ios/IOSPicker.mm) when several games
**   are found
** - IOS_FatalError: an alert with the error before the app ends
** - the log in Files (ECWolf/wolf3d-log.txt), the screen's size, the 60 FPS
**   frame cap
**
** The first screen, the picker and the alert run outside the game loop:
** they show UIKit and spin the main run loop until they are answered.
*/

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "SDL.h"
#include "SDL_main.h"

#include "filesys.h"
#include "zstring.h"
#include "wl_iwad.h"

#include "IOSGameData.h"
#include "IOSPicker.h"
#include "IOSTouch.h"

// ECWolf's own folder in Documents: settings, saves, screenshots, the log
static NSString* const kECWolfFolder = @"ECWolf";

static NSString* DocumentsPath()
{
	return NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
}

extern "C" const char* IOSGameData_Documents(void)
{
	static char path[1024];
	if (!path[0]) snprintf(path, sizeof(path), "%s", DocumentsPath().fileSystemRepresentation);
	return path;
}

// ---------------------------------------------------------------- paths

namespace FileSys {

// ECWolf asks for its Configuration folder (its settings), Documents (and
// in it Savegames and Screenshots), Application Support: on iOS all in
// Documents/ECWolf, which Files shows and which is kept with the app's
// data, but Application Support (nothing the player needs to see)
FString OSX_FindFolder(ESpecialDirectory dir)
{
	NSFileManager* fm = [NSFileManager defaultManager];
	NSString* path = nil;
	if (dir == DIR_ApplicationSupport)
		path = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
	else if (dir == DIR_Configuration)
		path = [DocumentsPath() stringByAppendingPathComponent:kECWolfFolder];
	else
		path = DocumentsPath(); // (ECWolf adds "/ECWolf" to Documents itself)
	if (path == nil) return FString();
	[fm createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:nil];
	return FString(path.fileSystemRepresentation);
}

}

// ---------------------------------------------------------------- alerts

static UIWindow* ownWindow = nil;

static UIWindowScene* AppScene()
{
	for (UIScene* scene in UIApplication.sharedApplication.connectedScenes)
		if ([scene isKindOfClass:UIWindowScene.class]) return (UIWindowScene*)scene;
	return nil;
}

static UIViewController* TopViewController()
{
	UIWindow* key = nil;
	UIWindowScene* scene = AppScene();
	for (UIWindow* w in scene.windows)
		if (w.isKeyWindow && !w.hidden) { key = w; break; }
	if (!key)
		for (UIWindow* w in scene.windows)
			if (!w.hidden && w.rootViewController) key = w;
	UIViewController* vc = key.rootViewController;
	while (vc.presentedViewController) vc = vc.presentedViewController;
	return vc;
}

// A plain black window of our own, over everything (for the picker and the
// alerts before the game's window, or over it)
static UIViewController* OwnController()
{
	if (!ownWindow)
	{
		UIWindowScene* scene = AppScene();
		ownWindow = scene ? [[UIWindow alloc] initWithWindowScene:scene] : [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
		ownWindow.rootViewController = [UIViewController new];
		ownWindow.rootViewController.view.backgroundColor = UIColor.blackColor;
		ownWindow.windowLevel = UIWindowLevelNormal + 1;
	}
	ownWindow.hidden = NO;
	[ownWindow makeKeyAndVisible];
	return ownWindow.rootViewController;
}

static void HideOwnWindow()
{
	if (ownWindow)
	{
		ownWindow.hidden = YES;
		ownWindow = nil;
	}
}

static void SpinRunLoop(double seconds)
{
	@autoreleasepool
	{
		[[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:seconds]];
	}
}

// Shows the alert and waits for an answer; returns the chosen action's index
// (-1: it couldn't be shown)
static int RunAlert(UIAlertController* alert, NSArray<NSString*>* titles)
{
	__block int chosen = -1;
	for (NSUInteger i = 0; i < titles.count; i++)
	{
		const int index = (int)i;
		[alert addAction:[UIAlertAction actionWithTitle:titles[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction*) { chosen = index; }]];
	}
	UIViewController* vc = TopViewController();
	if (vc == nil) vc = OwnController();
	if (alert.popoverPresentationController)
	{
		alert.popoverPresentationController.sourceView = vc.view;
		alert.popoverPresentationController.sourceRect = CGRectMake(CGRectGetMidX(vc.view.bounds), CGRectGetMidY(vc.view.bounds), 0, 0);
		alert.popoverPresentationController.permittedArrowDirections = 0;
	}
	[vc presentViewController:alert animated:YES completion:nil];
	if (alert.presentingViewController == nil)
	{
		HideOwnWindow();
		return -1;
	}
	while (chosen < 0) SpinRunLoop(0.05);
	for (int i = 0; i < 20 && alert.presentingViewController != nil; i++) SpinRunLoop(0.05);
	HideOwnWindow();
	return chosen;
}

void IOS_FatalError(const char* text)
{
	@autoreleasepool
	{
		IOSTouch_Hide();
		NSString* msg = [NSString stringWithUTF8String:text ? text : ""];
		if (msg == nil) msg = [NSString stringWithCString:text encoding:NSISOLatin1StringEncoding];
		UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Wolf3D: error" message:msg
			preferredStyle:UIAlertControllerStyleAlert];
		RunAlert(alert, @[ @"Exit" ]);
	}
}

// ---------------------------------------------------------------- the game picker

// ECWolf's picker on Apple systems (wl_iwad_picker.cpp): every time several
// games are found, the cards (IOSPicker.mm); a plain list if they can't be
// shown. -1: none picked (the app ends).
int I_PickIWad_Cocoa(WadStuff* wads, int numwads, bool showwin, int defaultiwad)
{
	(void)showwin;
	@autoreleasepool
	{
		IOSTouch_Hide();
		const char* names[64];
		const int ct = numwads < 64 ? numwads : 64;
		for (int i = 0; i < ct; i++) names[i] = wads[i].Name.GetChars();
		int chosen = IOSPicker_Run(names, ct, defaultiwad);
		if (chosen == IOSPICKER_UNAVAILABLE)
		{
			NSMutableArray<NSString*>* titles = [NSMutableArray array];
			for (int i = 0; i < ct; i++) [titles addObject:[NSString stringWithUTF8String:names[i]] ?: @"?"];
			[titles addObject:@"Quit"];
			UIAlertController* sheet = [UIAlertController alertControllerWithTitle:@"Wolf3D" message:@"Choose a game"
				preferredStyle:UIAlertControllerStyleActionSheet];
			chosen = RunAlert(sheet, titles);
			if (chosen >= ct) chosen = -1;
		}
		printf("Game picker: chose %d of %d\n", chosen, ct);
		fflush(stdout);
		return chosen;
	}
}

// ---------------------------------------------------------------- the first screen

static BOOL GameDataFound()
{
	static char folders[IOSGAMEDATA_MAX][IOSGAMEDATA_PATH];
	return IOSGameData_Folders(IOSGameData_Documents(), folders, IOSGAMEDATA_MAX) > 0;
}

@interface IOSGameDataViewController : UIViewController
@property (nonatomic) BOOL ready;
@property (nonatomic, strong) UILabel* missingLabel;
@end

@implementation IOSGameDataViewController

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.view.backgroundColor = UIColor.blackColor;

	UILabel* title = [UILabel new];
	title.text = [NSString stringWithUTF8String:IOSGameData_HelpTitle()];
	title.font = [UIFont systemFontOfSize:28 weight:UIFontWeightBold];
	title.textColor = UIColor.whiteColor;

	NSString* text = [NSString stringWithUTF8String:IOSGameData_HelpText()];
	if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad)
		text = [text stringByReplacingOccurrencesOfString:@"On My iPhone > Wolf3D (On My iPad on an iPad)" withString:@"On My iPad > Wolf3D"];
	else
		text = [text stringByReplacingOccurrencesOfString:@" (On My iPad on an iPad)" withString:@""];
	UILabel* body = [UILabel new];
	body.text = text;
	body.font = [UIFont systemFontOfSize:17];
	body.textColor = [UIColor colorWithWhite:0.85 alpha:1];
	body.numberOfLines = 0;

	self.missingLabel = [UILabel new];
	self.missingLabel.font = [UIFont systemFontOfSize:15 weight:UIFontWeightSemibold];
	self.missingLabel.textColor = [UIColor colorWithRed:1 green:0.35 blue:0.3 alpha:1];
	self.missingLabel.numberOfLines = 0;

	UIButton* check = [UIButton buttonWithType:UIButtonTypeSystem];
	[check setTitle:@"Check Again" forState:UIControlStateNormal];
	check.titleLabel.font = [UIFont systemFontOfSize:20 weight:UIFontWeightSemibold];
	[check setTitleColor:UIColor.blackColor forState:UIControlStateNormal];
	check.backgroundColor = [UIColor colorWithRed:0.85 green:0.15 blue:0.1 alpha:1];
	[check setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	check.layer.cornerRadius = 10;
	[check addTarget:self action:@selector(checkAgain) forControlEvents:UIControlEventTouchUpInside];
	[check.widthAnchor constraintGreaterThanOrEqualToConstant:180].active = YES;
	[check.heightAnchor constraintEqualToConstant:48].active = YES;

	UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[ title, body, self.missingLabel, check ]];
	stack.axis = UILayoutConstraintAxisVertical;
	stack.alignment = UIStackViewAlignmentLeading;
	stack.spacing = 16;
	stack.translatesAutoresizingMaskIntoConstraints = NO;

	UIScrollView* scroll = [UIScrollView new];
	scroll.translatesAutoresizingMaskIntoConstraints = NO;
	[scroll addSubview:stack];
	[self.view addSubview:scroll];

	UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
	[NSLayoutConstraint activateConstraints:@[
		[scroll.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:24],
		[scroll.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-24],
		[scroll.topAnchor constraintEqualToAnchor:safe.topAnchor constant:16],
		[scroll.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-16],
		[stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor],
		[stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor],
		[stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor],
		[stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor],
		[stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor],
	]];

	// coming back from the Files app checks again by itself
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(checkAgain)
		name:UIApplicationDidBecomeActiveNotification object:nil];
	[self checkAgain];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

- (void)checkAgain
{
	if (GameDataFound())
	{
		self.ready = YES;
		return;
	}
	self.missingLabel.text = @"Not found yet: no *.WL6, *.SOD, *.SD2 or *.SD3 files";
}

@end

// Before the game: ECWolf's folder (so the app shows in Files at once), the
// sound session, and the first screen until game files are found
static void IOS_AppInit()
{
	@autoreleasepool
	{
		// The game's sound plays with the phone on silent, as in the owner's
		// other ports, and takes over from other apps' music
		[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
		[[AVAudioSession sharedInstance] setActive:YES error:nil];
		[[NSFileManager defaultManager] createDirectoryAtPath:[DocumentsPath() stringByAppendingPathComponent:kECWolfFolder]
			withIntermediateDirectories:YES attributes:nil error:nil];
		if (GameDataFound()) return;

		printf("iOS: no game files yet: the first screen\n");
		fflush(stdout);
		UIWindowScene* scene = AppScene();
		UIWindow* window = scene ? [[UIWindow alloc] initWithWindowScene:scene] : [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
		IOSGameDataViewController* vc = [IOSGameDataViewController new];
		window.rootViewController = vc;
		window.windowLevel = UIWindowLevelNormal + 1;
		[window makeKeyAndVisible];
		while (!vc.ready)
		{
			@autoreleasepool
			{
				CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, true);
			}
		}
		window.hidden = YES;
		window.rootViewController = nil;
		printf("iOS: game files found\n");
		fflush(stdout);
	}
}

// ---------------------------------------------------------------- the screen

// The screen's own pixels, landscape: what the game draws at (ECWolf's full
// screen resolution, c_cvars.cpp). A big iPad's is brought down to 1440
// rows (the game draws in software), keeping its shape.
extern "C" void IOS_ScreenPixels(int* w, int* h)
{
	const CGRect b = UIScreen.mainScreen.nativeBounds;
	double pw = fmax(b.size.width, b.size.height), ph = fmin(b.size.width, b.size.height);
	if (ph > 1440)
	{
		pw = pw * 1440 / ph;
		ph = 1440;
	}
	*w = (int)pw & ~1;
	*h = (int)ph & ~1;
}

// The tray's FPS at 60 on a 120 Hz screen: frames kept at most 1/60 s
// apart (the waits save battery). (From the UZDoom and Raze ports.)
extern "C" void IOS_FrameCap(void)
{
	static uint64_t next = 0;
	const int fps = IOSTouch_MaxFps();
	if (fps <= 0 || fps >= 120) { next = 0; return; }
	const uint64_t period = 1000000000ull / (uint64_t)fps;
	const uint64_t now = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
	if (next == 0 || now > next + period) next = now; // (far behind, after a pause or a load: from now on)
	if (now < next)
	{
		const uint64_t wait = next - now;
		struct timespec ts = { (time_t)(wait / 1000000000ull), (long)(wait % 1000000000ull) };
		nanosleep(&ts, nullptr);
	}
	next += period;
}

// ---------------------------------------------------------------- the log

// Everything the game prints goes to Files > On My iPhone > Wolf3D > ECWolf
// > wolf3d-log.txt (replaced at each launch), so a problem on the phone can
// be read and sent. Runs before main().
__attribute__((constructor(101))) static void IOS_LogToFile()
{
	@autoreleasepool
	{
		NSString* dir = [DocumentsPath() stringByAppendingPathComponent:kECWolfFolder];
		[[NSFileManager defaultManager] createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
		const char* path = [[dir stringByAppendingPathComponent:@"wolf3d-log.txt"] fileSystemRepresentation];
		if (freopen(path, "w", stdout)) setvbuf(stdout, NULL, _IOLBF, 0);
		dup2(fileno(stdout), fileno(stderr));
		setvbuf(stderr, NULL, _IONBF, 0);
		NSDictionary* info = [[NSBundle mainBundle] infoDictionary];
		printf("Wolf3D for iOS, build %s (%s)\n", [[info objectForKey:@"SWBuildNumber"] description].UTF8String ?: "dev",
			[[info objectForKey:@"SWBuildDate"] description].UTF8String ?: "-");
		NSArray* items = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:DocumentsPath() error:nil];
		printf("Documents: %d items\n", (int)items.count);
		for (NSString* name in items) printf("  %s\n", name.UTF8String);
		fflush(stdout);
	}
}

// ---------------------------------------------------------------- main

extern int WL_Main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	// (as ECWolf's own main: numbers read and written the C way)
	setenv("LC_NUMERIC", "C", 1);
	SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");
	// Touches are the overlay's (IOSTouch.m): never mouse clicks
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "2");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	IOS_AppInit();
	// (ECWolf also looks for game files in the folder it runs in)
	chdir(IOSGameData_Documents());
	return WL_Main(argc, argv);
}
