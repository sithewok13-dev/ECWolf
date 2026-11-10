// The game picker: see IOSPicker.h. (Objective-C++ with ARC; no engine
// headers.) In the style of the owner's UZDoom and Raze ports' pickers.
//
// A card for each game found, side by side (scrolling where they don't all
// fit), each in its own colours with an emblem, its name and subtitle, and
// "LAST" on the one played last. A tap starts it.

#import <UIKit/UIKit.h>
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

- (instancetype)initWithName:(NSString*)name family:(int)fam last:(BOOL)last
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
		l.text = @"LAST";
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
@end

@implementation IOSPickController
{
	UIScrollView* scroll;
	NSMutableArray<IOSPickCard*>* cards;
	UILabel* heading;
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
	heading.text = @"Choose a game";
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
		IOSPickCard* c = [[IOSPickCard alloc] initWithName:self.names[i] family:FamilyOfName(self.names[i]) last:(i == self.last)];
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
	heading.frame = CGRectMake(left + 4, top, b.size.width - left - right, 32);
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
