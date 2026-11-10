# Wolf3D for iOS (ECWolf): notes for Claude Code sessions

An unofficial iPhone/iPad build of ECWolf playing Wolfenstein 3D, Spear of
Destiny and its two mission packs, app name **Wolf3D**, bundle id
`io.github.sithewok13.wolf3d`. The owner plays it on an iPhone (iOS 18+),
isn't a programmer, and reads everything on a phone. They own the GOG
versions (Wolfenstein 3D, Spear of Destiny with its mission packs).

- **Build branch:** `ios`. `master` is left as upstream (ECWolf).
- **Stage 1 (this):** an IPA that installs and plays: SDL2 renderer, full
  screen landscape (`IOSGame_GameResolution` in `ios/IOSGame.cpp`: ECWolf
  only lays out 4:3, 16:10, 16:9 and 64:27, so the game draws at whichever
  fills most of the screen above the home indicator, 64:27 full width on a
  Dynamic Island iPhone, at half the screen's pixels; it's the first entry
  of the mode list and overrides a saved resolution; `IOSGame_PictureRect`
  places it at exactly 2x, nearest, above the home indicator), sound and
  music, settings, saves and the
  log (`wolf3d-log.txt`) in Documents/ECWolf (Files shows them). No game
  files: a first screen says where to copy them and starts the game once
  they are found. The game files go anywhere in Files > On My iPhone >
  Wolf3D, two folder levels down at most (`ios/IOSGameData.cpp`); GOG's
  layout works as copied: Wolfenstein 3D's *.WL6 at the top and SoD in `m1`;
  SoD's `M1`, and the mission packs in `M2`/`M3` with *.SOD names, named
  .sd2/.sd3 in memory only (`IOS_NameMissionPack` in `src/wl_iwad.cpp`: a
  short MAPHEAD, then the folder's name or GAMEMAPS' size). Several games
  found: the picker's cards (`ios/IOSPicker.mm`). ECWolf's own `ecwolf.pk3`
  (from `wadsrc/`) is bundled; it is not game data.
- **Later stages:** CAMERA, the sithewok cheat, AUTOPLAY (as in the owner's
  other ports); none of it is here yet.
- **iOS code:** `ios/`: `i_ios.mm` (main, paths, the first screen, the
  picker's fallback and the error alert, the log, the 60 FPS cap; SDL's accelerometer joystick is off: ECWolf read
  it as a stick and the view turned by itself),
  `IOSPicker.mm`, `IOSTouch.m`/`.h` (the overlay, C core + UIKit view, from
  the UZDoom port's), `IOSGame.cpp` (the game side), `IOSGameData.cpp`,
  `Info.plist.in`, `build_deps.sh` (SDL2, SDL2_mixer, SDL2_net). Engine
  changes are `#ifdef ECWOLF_IOS` (the app), `ECWOLF_TOUCH` (the touch
  controls: iOS and the rig) or `ECWOLF_IOSPATHS` (the data search): hooks
  in `id_in.cpp` (every event pump runs `IOSGame_Frame`), `wl_play.cpp`
  (the tic's command, the game held still by the wheel, quick save/load),
  `wl_loadsave.cpp` (`TouchQuickSave`/`TouchQuickLoad`: no question, the
  slot "Quicksave"), `sdlvideo.cpp`, `c_cvars.cpp` (iOS defaults: full
  screen, vsync, view size 20), `id_us_1.cpp` (the keyboard for typing a
  save's name), `r_sprites.cpp` (the wheel's pictures), `wl_main.cpp`.
- **Touch modes:** GAMEPLAY only in the frames the play loop pumps
  (`IOSGame_SetInPlay`); everywhere else (menus, title and story screens,
  intermissions, Y/N questions) MENUS: a tap is Return, a slide is arrow
  keys (one per 30 pt), BACK is Escape, ▲ ▼ on the right, MENU too. Keys go
  in as real SDL key events, one at a time (`PumpKeys`).
- **Frame rate:** ECWolf draws one frame per 70 Hz tic and doesn't
  interpolate. Wolf3D always runs at 60 (the owner's call: no 60/120
  switch): the screen is asked for 60 and frames are kept 1/60 s apart; the
  game's speed never changes (tics are counted from the clock). The log
  gives fps and ms per frame every 5 s of play (`LogFrameRate`).
- **CI:** `.github/workflows/ios.yml` (macos-15, newest Xcode): deps cached,
  host `zipdir` (TOOLS_ONLY) for the pk3, the iOS build, the icon from
  `src/macosx/icon.iconset`, `Wolf3D-ios.ipa`, a check that fails if game
  data is in the app, and the public `ios-latest` prerelease on pushes to
  `ios` (never Latest; body first line "Build <run> · <YYYY-MM-DD>"). The
  build stamp ("build 80 · Nov 9", SWBuildNumber/SWBuildDate in Info.plist)
  shows under the open MENU tray.
- **Docs:** `docs/ios/README.md` (install with Feather, where the files go).

## Testing without a Mac (the Linux rig)

- Deps: `DEPS_NATIVE=1 ios/build_deps.sh <dir>`. Configure the desktop build
  with `-DECWOLF_TOUCH_RIG_SOURCE=$PWD/ios/rig/IOSTouchRig.c -DNO_GTK=ON`,
  `-DCMAKE_DISABLE_FIND_PACKAGE_SDL2{,_mixer,_net}=ON` and
  `SDL2{,_mixer,_net}_LIBRARIES` / `_INCLUDE_DIRS` pointing at them.
- Run with `IOSRIG_DOCUMENTS=<folder standing in for Documents>`,
  `IOSRIG_SCRIPT=<script>` and `HOME=<scratch>` under Xvfb
  (`--res 1688 780`). A phone's screen: `IOSRIG_SCREEN="874 402 3 21"`
  (points wide, high, scale, home indicator inset) on an X screen of its
  pixels (`-screen 0 2622x1206x24`), no `--res`. With no game files it prints the first screen and
  exits 3. Script lines: `frame <n>`, `wait <n>`, touches (`down/move/up
  <id> <x y | @BUTTON>`, `tapslice`, `gyro`, `state`), `pos`, `key <SDL
  name>`, `shot <png>` (ImageMagick `import`), `quit`. The picker is the
  console's there: pipe the game's number in.
- Objective-C (the overlay's view, `i_ios.mm`, the picker) only compiles in
  CI. Gameplay on the phone is only tested by the owner.

## Rules

- **Game data:** never commit, upload, bundle or publish game files (*.WL1,
  *.WL3, *.WL6, *.SDM, *.SOD, *.SD1-3, *.N3D, installers, zips), here or
  anywhere public. Work on local copies only, outside the repository.
- **Commits:**
  - Identity `Claude <noreply@anthropic.com>` (already configured); never
    the owner's email or name. Subject `iOS: ...`, then a plain wrapped body.
  - End with the attribution trailer lines your session provides. Apart from
    those, no AI model names in commits, code or docs.
  - No PRs unless asked. Never stash, reset or check out over a working tree
    that another job may be editing. Push with `git push -u origin ios`;
    never force-push, never push tags (an `ios-v*` tag would be a release).
- **Releases:** no versioned release yet; each push to `ios` updates the
  public `ios-latest` prerelease (the owner's go). Before any other public
  release or public text, build a preview and wait for the owner's explicit
  go.
- **Touch controls standard** (shared with the owner's other ports; keep it
  the same):
  - Move stick where the left thumb lands; drag elsewhere to look (turn).
  - QUICK SAVE / QUICK LOAD: a 0.3 s hold with a ring. Sliding off cancels;
    QUICK SAVE wins a tie.
  - MENU: tap = game menu; hold 0.45 s = tray with SENS, GYRO, OPACITY, FPS,
    keyboard, and the build stamp under it in small dim text (not a button).
  - OPACITY (tray): tap = a slider under the tray, 20% to 100% in 5% steps,
    live, saved on lift (`iosTouchOpacity`, default 100%); hold = back to
    100%. The controls follow it; the tray and the wheel stay at full.
  - FPS (tray): tap = show/hide the FPS readout. Wolf3D differs from the
    other ports here (the owner's call): no 60/120 switch, no hold action,
    the label just "FPS"; the game runs at a fixed 60.
  - GYRO: OFF (default) / TOUCH (aims while either thumb is down; lifting
    both freezes the view) / ALWAYS. SENS 1.0/1.5/2.0/3.0, default 1.5.
    Remembered.
  - NEXT WPN: tap = next; a 10 pt slide opens the weapon wheel at once; a
    0.3 s hold opens it to tap. The gap at the bottom cancels. The game
    holds still while a wheel is open. A haptic tick per slice.
  - Wolf3D's buttons: FIRE, USE, MAP (automap), RUN (always run on/off, lit
    while on), NEXT WPN; the stick strafes sideways.
  - HUD and buttons stay clear of the corners, the notch or Dynamic Island,
    and the home bar.
- **Testing:** say plainly what was and wasn't tested. The owner tests on
  the device.
- **Lean mode:** small contexts, no full log or file dumps
  (grep/sed/head/tail), reuse builds.
- **Talking to the owner:** plain words, short messages, direct links, and
  honesty about mistakes and untested things. If they skip a question,
  choose a sensible default and say what you chose.
