// The game picker: see IOSPicker.h. (Objective-C++ with ARC; no engine
// headers.) In the style of the owner's UZDoom and Raze ports' pickers.
//
// A card for each game found, side by side (scrolling where they don't all
// fit), each in its own colours with an emblem, its name and subtitle, and
// "LAST" on the one played last. A tap starts it.
//
// "Switch game" (the tray's GAMES, IOSGame.cpp): the same cards over the
// game, "PLAYING" on the one being played, and Cancel. Picking another keeps
// it (its name, the time), writes the settings and closes the app with a
// local notification, "Tap to start <game>"; opening the app within 10
// minutes (by the notification or not) starts it straight away, no picker
// (IOSPicker_TakeNext). As the owner's Half-Life and Raze ports do.

#import <UIKit/UIKit.h>
#import <UserNotifications/UserNotifications.h>
#include <stdio.h>
#include <string.h>
#include "IOSPicker.h"

// ---------------------------------------------------------------- the games

enum { FAM_WOLF, FAM_SPEAR, FAM_DANGER, FAM_CHALLENGE, FAM_OTHER, FAM_COUNT };

typedef struct
{
	const char* title;     // the card's name
	const char* subtitle;  // under it
	const char* symbol;    // an SF Symbol, the card's emblem
	CGFloat top[3], bottom[3]; // the card's gradient
} Family;

static const Family families[FAM_COUNT] =
{
	{ "WOLFENSTEIN 3D", "Escape from Castle Wolfenstein", "shield.fill", { 0.62, 0.08, 0.06 }, { 0.18, 0.02, 0.02 } },
	{ "SPEAR OF DESTINY", "Operation: Spear", "bolt.fill", { 0.20, 0.30, 0.62 }, { 0.04, 0.07, 0.20 } },
	{ "RETURN TO DANGER", "Spear of Destiny · Mission 2", "exclamationmark.triangle.fill", { 0.15, 0.45, 0.25 }, { 0.03, 0.12, 0.06 } },
	{ "ULTIMATE CHALLENGE", "Spear of Destiny · Mission 3", "flame.fill", { 0.55, 0.32, 0.06 }, { 0.17, 0.08, 0.02 } },
	{ "", "", "square.stack.3d.up.fill", { 0.35, 0.35, 0.38 }, { 0.08, 0.08, 0.09 } },
};

// Which card a game ECWolf found goes on, by its name
static int FamilyOfName(NSString* name)
{
	NSString* n = name.lowercaseString;
	if ([n containsString:@"return to danger"]) return FAM_DANGER;
	if ([n containsString:@"ultimate challenge"]) return FAM_CHALLENGE;
	if ([n containsString:@"spear of destiny"] && ![n containsString:@"demo"]) return FAM_SPEAR;
	if ([n isEqualToString:@"wolfenstein 3d"]) return FAM_WOLF;
	return FAM_OTHER;
}

static UIColor* RGB(const CGFloat c[3], CGFloat a)
{
	return [UIColor colorWithRed:c[0] green:c[1] blue:c[2] alpha:a];
}

// ---------------------------------------------------------------- a card

@interface IOSPickCard : UIControl
@property (nonatomic) int index;
@end

@implementation IOSPickCard
{
	CAGradientLayer* back;
}

- (instancetype)initWithName:(NSString*)name family:(int)fam last:(BOOL)last mark:(NSString*)mark
{
	self = [super initWithFrame:CGRectZero];
	if (!self) return nil;
	const Family* f = &families[fam];
	back = [CAGradientLayer layer];
	back.colors = @[ (id)RGB(f->top, 1).CGColor, (id)RGB(f->bottom, 1).CGColor ];
	[self.layer addSublayer:back];
	self.layer.cornerRadius = 18;
	self.layer.borderWidth = 1.5;
	self.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.18].CGColor;
	self.clipsToBounds = YES;

	UIImageSymbolConfiguration* big = [UIImageSymbolConfiguration configurationWithPointSize:110 weight:UIImageSymbolWeightBold];
	UIImageView* emblem = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:[NSString stringWithUTF8String:f->symbol] withConfiguration:big]];
	emblem.tintColor = [UIColor colorWithWhite:1.0 alpha:0.10];
	emblem.tag = 1;
	[self addSubview:emblem];
	UIImageSymbolConfiguration* small = [UIImageSymbolConfiguration configurationWithPointSize:30 weight:UIImageSymbolWeightBold];
	UIImageView* badge = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:[NSString stringWithUTF8String:f->symbol] withConfiguration:small]];
	badge.tintColor = [UIColor colorWithWhite:1.0 alpha:0.9];
	badge.tag = 2;
	[self addSubview:badge];

	UILabel* title = [UILabel new];
	title.text = f->title[0] ? [NSString stringWithUTF8String:f->title] : name.uppercaseString;
	title.textColor = UIColor.whiteColor;
	title.font = [UIFont systemFontOfSize:21 weight:UIFontWeightHeavy];
	title.numberOfLines = 2;
	title.adjustsFontSizeToFitWidth = YES;
	title.minimumScaleFactor = 0.6;
	title.tag = 3;
	[self addSubview:title];
	UILabel* sub = [UILabel new];
	sub.text = f->subtitle[0] ? [NSString stringWithUTF8String:f->subtitle] : @"";
	sub.textColor = [UIColor colorWithWhite:1.0 alpha:0.8];
	sub.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
	sub.numberOfLines = 2;
	sub.tag = 4;
	[self addSubview:sub];
	if (last)
	{
		UILabel* l = [UILabel new];
		l.text = mark;
		l.textAlignment = NSTextAlignmentCenter;
		l.font = [UIFont systemFontOfSize:10 weight:UIFontWeightHeavy];
		l.textColor = UIColor.blackColor;
		l.backgroundColor = [UIColor colorWithRed:1.0 green:0.82 blue:0.35 alpha:1.0];
		l.layer.cornerRadius = 7;
		l.clipsToBounds = YES;
		l.tag = 5;
		[self addSubview:l];
	}
	for (UIView* v in self.subviews) v.userInteractionEnabled = NO;
	return self;
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	const CGSize s = self.bounds.size;
	back.frame = self.bounds;
	[self viewWithTag:1].center = CGPointMake(s.width - 40, s.height * 0.42);
	UIView* badge = [self viewWithTag:2];
	[badge sizeToFit];
	badge.frame = CGRectMake(16, 16, badge.bounds.size.width, badge.bounds.size.height);
	[self viewWithTag:3].frame = CGRectMake(16, s.height - 104, s.width - 32, 56);
	[self viewWithTag:4].frame = CGRectMake(16, s.height - 48, s.width - 32, 36);
	[self viewWithTag:5].frame = CGRectMake(s.width - 56, 14, 42, 16);
}

- (void)setHighlighted:(BOOL)highlighted
{
	[super setHighlighted:highlighted];
	self.transform = highlighted ? CGAffineTransformMakeScale(0.96, 0.96) : CGAffineTransformIdentity;
	self.alpha = highlighted ? 0.85 : 1.0;
}

@end

// ---------------------------------------------------------------- the screen

@interface IOSPickController : UIViewController
@property (nonatomic) int picked; // -1 until a card is tapped
@property (nonatomic, copy) NSArray<NSString*>* names;
@property (nonatomic) int last;
@property (nonatomic) BOOL switching; // "Switch game": the one playing marked, and Cancel
@property (nonatomic, copy) void (^done)(int picked); // (switching) a card or Cancel (-1)
@end

@implementation IOSPickController
{
	UIScrollView* scroll;
	NSMutableArray<IOSPickCard*>* cards;
	UILabel* heading;
	UIButton* cancel;
	CAGradientLayer* backdrop;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.picked = -1;
	self.view.backgroundColor = UIColor.blackColor;
	backdrop = [CAGradientLayer layer];
	backdrop.colors = @[ (id)[UIColor colorWithRed:0.14 green:0.04 blue:0.03 alpha:1].CGColor,
	                     (id)[UIColor colorWithRed:0.02 green:0.01 blue:0.01 alpha:1].CGColor ];
	[self.view.layer addSublayer:backdrop];

	heading = [UILabel new];
	heading.text = self.switching ? @"Switch game" : @"Choose a game";
	if (self.switching)
	{
		cancel = [UIButton buttonWithType:UIButtonTypeSystem];
		[cancel setTitle:@"Cancel" forState:UIControlStateNormal];
		cancel.titleLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
		[cancel setTitleColor:[UIColor colorWithRed:1.0 green:0.82 blue:0.35 alpha:1.0] forState:UIControlStateNormal];
		[cancel addTarget:self action:@selector(cancelTapped) forControlEvents:UIControlEventTouchUpInside];
		[self.view addSubview:cancel];
	}
	heading.textColor = [UIColor colorWithWhite:1.0 alpha:0.9];
	heading.font = [UIFont systemFontOfSize:20 weight:UIFontWeightSemibold];
	[self.view addSubview:heading];

	scroll = [UIScrollView new];
	scroll.showsHorizontalScrollIndicator = NO;
	scroll.alwaysBounceHorizontal = NO;
	[self.view addSubview:scroll];

	// the four known games in their own order, then any others
	cards = [NSMutableArray array];
	NSMutableArray<NSNumber*>* order = [NSMutableArray array];
	for (int fam = 0; fam < FAM_COUNT; fam++)
		for (int i = 0; i < (int)self.names.count; i++)
			if (FamilyOfName(self.names[i]) == fam) [order addObject:@(i)];
	for (NSNumber* n in order)
	{
		const int i = n.intValue;
		IOSPickCard* c = [[IOSPickCard alloc] initWithName:self.names[i] family:FamilyOfName(self.names[i]) last:(i == self.last)
			mark:(self.switching ? @"PLAYING" : @"LAST")];
		c.index = i;
		[c addTarget:self action:@selector(cardTapped:) forControlEvents:UIControlEventTouchUpInside];
		[scroll addSubview:c];
		[cards addObject:c];
	}
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];
	backdrop.frame = self.view.bounds;
	const UIEdgeInsets in = self.view.safeAreaInsets;
	const CGRect b = self.view.bounds;
	const CGFloat left = fmax(in.left, 16), right = fmax(in.right, 16);
	const CGFloat top = fmax(in.top, 12), bottom = fmax(in.bottom, 12);
	heading.frame = CGRectMake(left + 4, top, b.size.width - left - right - 100, 32);
	cancel.frame = CGRectMake(b.size.width - right - 90, top, 90, 32);
	scroll.frame = CGRectMake(0, top + 40, b.size.width, b.size.height - top - 40 - bottom);
	const CGFloat gap = 14, h = fmin(scroll.frame.size.height - 8, 300);
	const NSInteger n = cards.count;
	const CGFloat avail = b.size.width - left - right;
	CGFloat w = n > 0 ? (avail - gap * (n - 1)) / n : avail;
	w = fmax(150, fmin(w, 230));
	const CGFloat total = n * w + (n - 1) * gap;
	CGFloat x = total < avail ? left + (avail - total) * 0.5 : left;
	const CGFloat y = (scroll.frame.size.height - h) * 0.5;
	for (IOSPickCard* c in cards)
	{
		c.frame = CGRectMake(x, y, w, h);
		x += w + gap;
	}
	scroll.contentSize = CGSizeMake(fmax(x - gap + right, b.size.width), scroll.frame.size.height);
}

- (void)cardTapped:(IOSPickCard*)card
{
	[[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium] impactOccurred];
	self.picked = card.index;
	if (self.done) self.done(card.index);
}

- (void)cancelTapped
{
	if (self.done) self.done(-1);
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
	return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

@end

int IOSPicker_Run(const char* const* names, int ctNames, int iLast)
{
	if (ctNames <= 0) return IOSPICKER_UNAVAILABLE;
	@autoreleasepool
	{
		UIWindowScene* scene = nil;
		for (UIScene* s in UIApplication.sharedApplication.connectedScenes)
			if ([s isKindOfClass:UIWindowScene.class]) { scene = (UIWindowScene*)s; break; }
		UIWindow* window = scene ? [[UIWindow alloc] initWithWindowScene:scene] : [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
		if (window == nil) return IOSPICKER_UNAVAILABLE;
		NSMutableArray<NSString*>* list = [NSMutableArray array];
		for (int i = 0; i < ctNames; i++) [list addObject:[NSString stringWithUTF8String:names[i] ? names[i] : "?"] ?: @"?"];
		IOSPickController* vc = [IOSPickController new];
		vc.names = list;
		vc.last = iLast;
		window.rootViewController = vc;
		window.windowLevel = UIWindowLevelNormal + 2;
		[window makeKeyAndVisible];
		while (vc.picked < 0)
		{
			@autoreleasepool
			{
				CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
			}
		}
		const int picked = vc.picked;
		window.hidden = YES;
		window.rootViewController = nil;
		return picked;
	}
}

// ---------------------------------------------------------------- switching

#define KEY_NEXT_NAME @"wolf3dNextGameName"
#define KEY_NEXT_TIME @"wolf3dNextGameTime"
#define NEXT_GAME_TIME 600.0 // seconds

int IOSPicker_TakeNext(const char* const* names, int ctNames)
{
	// its "tap to start" notification, if still there, has done its job
	[[UNUserNotificationCenter currentNotificationCenter] removeAllDeliveredNotifications];
	NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
	NSString* name = [d stringForKey:KEY_NEXT_NAME];
	const double age = NSDate.timeIntervalSinceReferenceDate - [d doubleForKey:KEY_NEXT_TIME];
	[d removeObjectForKey:KEY_NEXT_NAME];
	[d removeObjectForKey:KEY_NEXT_TIME];
	if (!name || age < 0 || age > NEXT_GAME_TIME) return -1;
	for (int i = 0; i < ctNames; i++)
	{
		if (names[i] && [name isEqualToString:@(names[i])])
		{
			printf("Game picker: the game switched to, %s\n", names[i]);
			return i;
		}
	}
	return -1;
}

static UIViewController* TopController(void)
{
	UIWindow* key = nil;
	for (UIScene* s in UIApplication.sharedApplication.connectedScenes)
	{
		if (![s isKindOfClass:UIWindowScene.class]) continue;
		for (UIWindow* w in ((UIWindowScene*)s).windows)
			if (w.isKeyWindow) key = w;
		if (!key) key = ((UIWindowScene*)s).windows.firstObject;
	}
	UIViewController* top = key.rootViewController;
	while (top.presentedViewController) top = top.presentedViewController;
	return top;
}

// The pick kept, the settings written, a "Tap to start" notification asked
// for, and the app closed
static void CloseForNext(NSString* title, void (*pfnSave)(void))
{
	NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
	[d setObject:title forKey:KEY_NEXT_NAME];
	[d setDouble:NSDate.timeIntervalSinceReferenceDate forKey:KEY_NEXT_TIME];
	[d synchronize];
	if (pfnSave) pfnSave();
	printf("Switch game: %s, the app closes\n", title.UTF8String);
	fflush(stdout);
	UNUserNotificationCenter* center = UNUserNotificationCenter.currentNotificationCenter;
	[center requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionSound
		completionHandler:^(BOOL granted, NSError* err) {
		if (!granted)
		{
			// no notifications: say how it goes instead, then close
			dispatch_async(dispatch_get_main_queue(), ^{
				UIAlertController* alert = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"Open Wolf3D again to start %@", title]
					message:@"The app closes now. Your pick starts when you open it again (within 10 minutes)." preferredStyle:UIAlertControllerStyleAlert];
				[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* a) { exit(0); }]];
				UIViewController* top = TopController();
				if (top) [top presentViewController:alert animated:YES completion:nil];
				else exit(0);
			});
			return;
		}
		UNMutableNotificationContent* content = [UNMutableNotificationContent new];
		content.title = [NSString stringWithFormat:@"Tap to start %@", title];
		content.body = @"Or open Wolf3D within 10 minutes.";
		UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"wolf3d.nextgame" content:content
			trigger:[UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO]];
		[center addNotificationRequest:request withCompletionHandler:^(NSError* e2) {
			dispatch_async(dispatch_get_main_queue(), ^{ exit(0); });
		}];
	}];
}

void IOSPicker_PresentSwitch(const char* const* names, int ctNames, int iNow, void (*pfnSave)(void), void (*pfnDone)(void))
{
	UIViewController* host = TopController();
	if (!host || ctNames <= 0)
	{
		if (pfnDone) pfnDone();
		return;
	}
	NSMutableArray<NSString*>* list = [NSMutableArray array];
	for (int i = 0; i < ctNames; i++) [list addObject:[NSString stringWithUTF8String:names[i] ? names[i] : "?"] ?: @"?"];
	IOSPickController* menu = [IOSPickController new];
	menu.names = list;
	menu.last = iNow;
	menu.switching = YES;
	menu.modalPresentationStyle = UIModalPresentationFullScreen;
	__weak IOSPickController* weakMenu = menu;
	menu.done = ^(int picked) {
		if (picked >= 0 && picked < (int)list.count && picked != iNow)
		{
			CloseForNext(list[picked], pfnSave);
			return; // (the cards stay up until the app closes)
		}
		[weakMenu dismissViewControllerAnimated:YES completion:nil];
		if (pfnDone) pfnDone();
	};
	[host presentViewController:menu animated:YES completion:nil];
}
