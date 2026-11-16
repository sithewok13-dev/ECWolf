# Wolf3D for iPhone and iPad

An unofficial build of [ECWolf](https://maniacsvault.net/ecwolf/) for iPhone
and iPad, with touch controls. It plays your own copy of Wolfenstein 3D,
Spear of Destiny and the two Spear of Destiny mission packs (Return to
Danger, Ultimate Challenge). No game files come with the app.

## Install

You need an iPhone or iPad with iOS 15 or newer. The app isn't on the App
Store, so you install it yourself ("sideloading").

### Easiest: add the source in SideStore or AltStore

1. Set up [SideStore](https://sidestore.io) or
   [AltStore](https://altstore.io) (AltStore Classic; AltStore PAL can't
   install it), with its own instructions.
2. On your iPhone, tap
   [Add to SideStore](https://sithewok13-dev.github.io/wolf3d/#add-sidestore)
   or [Add to AltStore](https://sithewok13-dev.github.io/wolf3d/#add-altstore).
   Or open the app, go to *Sources*, tap **+** and paste this address:
   `https://raw.githubusercontent.com/sithewok13-dev/ECWolf/ios/packaging/ios/altstore-source.json`
3. Install **Wolf3D** from the source. Updates come the same way.

### Or download the IPA

1. Download `Wolf3D-ios.ipa` from the
   [releases page](https://github.com/sithewok13-dev/ECWolf/releases): the
   newest numbered release, or the *iOS latest build* (rebuilt after every
   change, before anyone has tried it).
2. Open it in Feather, Sideloadly or another signing tool and install it.

With a free Apple ID, sideloaded apps stop opening after 7 days until you
refresh them in your tool, and you can have at most 3 at once.

## Your game files

1. Open the Wolf3D app once, so its folder shows up in the Files app.
2. In Files, go to On My iPhone > Wolf3D and copy your game folders in:
   - GOG's Wolfenstein 3D: the whole folder (the *.WL6 files; Spear of
     Destiny is in its `m1` folder).
   - GOG's Spear of Destiny: the whole folder, with `M1`, `M2` and `M3` in
     it (the mission packs: nothing needs renaming).
   - Files from other copies (Steam, CD) work too: *.WL6, *.SOD, *.SD2,
     *.SD3.
3. Go back to Wolf3D: it starts by itself. With several games, pick one on
   the cards.

Your settings, saves and the log (`wolf3d-log.txt`) are in Files > On My
iPhone > Wolf3D > ECWolf.

## Controls

- Left thumb anywhere on the left: the move stick (sideways strafes).
  Slide your thumb up onto the small ^^ above it to run.
- Drag anywhere else: turn.
- FIRE and USE (doors, switches) bottom right, above the status bar; NEXT
  WPN (slide or hold it for the weapon wheel) and MAP (the automap) top
  left.
- SAVE and LOAD: tap for the game's Save Game / Load Game menu; hold 0.3 s
  for a quick save / quick load.
- MENU: tap for the game's menu; hold for the tray: LOOK (turning speed,
  0.5x to 3x; hold it for 1x), ALWAYS RUN (the stick always runs), HUD (the
  HUD's look, size and opacity), FPS (shows or hides the frame counter; the
  game always runs at 60 frames a second), the camera symbol (shows or
  hides CAMERA; hold it for the camera's options), CHEAT, LEVEL (a list of
  the game's levels), GAMES (with more than one game: switch game) and the
  keyboard.
- CAMERA (top, in the middle): tap for a photo, hold for a selfie, slide
  sideways for a burst, down for Action Freeze, up for a cinematic burst.
  Pictures go to Photos.
- CHEAT (the tray's, or type "sithewok"): god mode, every weapon and key,
  ammo kept full. The CHEAT button at the top: hold it to switch the cheat
  on or off. While it's on, hold USE for noclip (through walls).
- In the menus and on the title screens: tap to select or go on, slide up
  or down (or the arrows) to move, BACK to go back.

## More

- Every control in detail, known limitations and what was tested: the
  [release notes](https://github.com/sithewok13-dev/ECWolf/blob/ios/docs/ios/RELEASE_NOTES.md).
- No multiplayer: Wolfenstein 3D is a single-player game, and ECWolf's early
  multiplayer test code can only be started from a computer's command line.
- Found a bug? [Report it here](https://github.com/sithewok13-dev/ECWolf/issues),
  not to upstream ECWolf, with `wolf3d-log.txt` (in Files > On My iPhone >
  Wolf3D > ECWolf) and the build number (hold MENU: it's under the tray).
