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
  places it at exactly 2x, nearest, above the home indicator; on a
  Dynamic Island iPhone that is the full width, with a 12 pt strip above
  and the home indicator's below: the screen above it is 2.29:1, a little
  taller than 64:27, the widest shape ECWolf lays out; the status bar's
  teal sides are ECWolf's own widescreen look), sound and
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
- **Stage 2a (build 3):** no gyro (the owner's call: no tilt aiming, no
  GYRO or SENS); LOOK in the tray; the run marker on the stick and ALWAYS
  RUN in the tray (no RUN button); MAP top left; SAVE / LOAD (tap = the
  game's menu, hold = quick); the sithewok cheat. **Stage 2b:** CAMERA
  (below). **Later:** AUTOPLAY (as in the owner's other ports).
- **CAMERA** (top row, middle; single player): tap = photo (the tray's
  CAMERA held opens its options: PHOTO kinds clean / HUD / HUD + buttons,
  SELFIES 1/3/5, FREEZE on/off), hold 0.5 s (ring) = selfie, slide sideways
  = burst (10 clean, 0.2 s apart), slide down = Action Freeze, slide up =
  the cinematic burst (5). The tray's CAMERA (camera symbol) shows/hides it
  (`iosTouchShowCamera`). Photos are the game's own 8-bit pixels made RGBA
  with the base palette and scaled up a whole number of times, nearest (2x
  on a phone): clean = the view window before the weapon, HUD = the whole
  screen. Saved to Photos (add-only, `NSPhotoLibraryAddUsageDescription`)
  else Files > Wolf3D > Screenshots; a flash, shutter sound and tap each
  (`ios/IOSPhotos.mm`, from the Raze port's iosPhotos.mm; messages on the
  overlay's line). `ios/IOSCapture.cpp` takes the frames;
  `ios/IOSCamera.cpp` is the game side: a photo view drawn from its own
  camera (hooks in `wl_draw.cpp`: `CalcViewVariables`, `WallRefresh`'s
  viewz/viewshift, `CalcRotate`, no weapon, `IOSCamera_RenderBegin/End`
  around the render), the game held still (`IOSGame_HoldsGame`), and
  everything it changes (people moved/posed/hidden, BJ's figure on the
  player's body, the lens's FOV, map `visible`/`amFlags`, actors'
  projection fields) applied only while the view is drawn and put back
  right after. Selfie = a group photo from the player's phone: BJ himself
  big in a bottom corner (build 7, the owner's ask), as the one holding it:
  his status-bar face from the player's data (`STFST00`, the grin
  `STFEVL0`, Spear's golden-eyed `STFGOD0`, now and then `STFST01/02`),
  its grey panel and blue line keyed out, scaled up a whole number of
  times (nearest, 50-80% of the view's height), on shoulders in his grey
  shirt (the victory run's) drawn in the same big pixels (`ShirtAt`), up
  to a third of the face off the frame's edge; side, size, face, mirror
  and shirt per photo (`PickSelf`), drawn into the 3D view after the scene
  (`DrawSelf` from `IOSCamera_AfterScene`; the screen is drawn afresh each
  frame), the view turned a little his way so the group stands clear of
  him (`SelfShift`). Only enemies where the player is
  (`HereWithPlayer`): his area or one joined to it by open doors
  (ECWolf's sound zones, `CheckLink`) within 14 tiles, or in sight within
  20 -- never anyone from elsewhere in the map; the nearest 12 of them
  brought in front (the way with room), even just one;
  standing frame (`SpawnState`) or a step (`SeeState`), facing the lens,
  an arrangement per photo (huddle, peek, class rows, corner, tiny,
  totem, pyramid, lift: lifts via actor `z`, 0.58 a shoulder); other
  enemies, shots and whatever stands in front of a face hidden; "No one
  around for a group photo" / "No room for a group photo here". Freeze:
  orbits the enemy in the middle of the view or a point ahead (against a
  wall: BJ, from behind), keeps out of walls/closed doors, height 0.06-0.94,
  tilt as a y-shear (at most a third of the view), pinch: lens zoom to 8x
  (FOV), below 1x backs off. BJ is the victory run's `BLAZ` sprites
  (`BJRun`: running in freeze, the jump's arms-up frame in cinema); Spear
  has none, so he isn't drawn there. Rig: `cam photo|selfie|burst|freeze|
  cinema|end|orbit y p|pan r d|zoom f|kinds n|selfies n|hash|status`,
  and, changing the game (rig only), `cam goto x y angle` (the nearest
  open tile), `cam who` (enemies near: area, sight, taken or not), `cam
  groups` (areas with 3+ enemies), `cam gather n`;
  `@CAMERA`, `@TRAYCAM`; photos in `$IOSCAM_DIR`; `cam hash` (actors,
  map, clock, every RNG) is printed before and after each held stage.
- **HUD** (tray's HUD: SPLIT -> STRIP -> CLASSIC -> NONE, default SPLIT,
  tap only; FACE on/off, default on; NSUserDefaults `iosTouchHud` 0-3,
  `iosTouchHudFace`; the owner's approved mockups, from the UZDoom port's):
  SPLIT, STRIP and NONE give the 3D view the whole screen (view size 21,
  no status bar; CLASSIC view size 20; set each play-loop pass from the
  HUD, `HudViewSize` in `IOSGame_CheckKeys`). SPLIT draws two dark rounded
  pills at the bottom: left [BJ's face] | first-aid kit + health % | the
  extra life + lives; right the gun in hand (its status-bar picture,
  recoloured light grey) + ammo | the keys held. FIRE and USE sit 14 pt
  above them. STRIP: one pill in the middle, shrunk to fit between FIRE/
  USE and the middle. CLASSIC: ECWolf's status bar, FIRE and USE above it
  (its top from the game, `fStatusTop`). NONE: nothing. Warnings: health
  under 25 red, with a red glow pulsing twice a second (and the game's own
  bloodied face); ammo red strictly below 25% of its maximum, or at 0.
  One size, the owner's "large": the pill is 83 px at 2000x921 (39.8 pt of
  a 440 pt screen, 1.3x the mockups'; its bottom where the mockups' was),
  scaled with the screen's height (0.8-1.25x), inside the safe frame and
  3 pt inside the game's picture (`IOSTouchHud.fPictureBottom`: the picture
  stops above the home indicator, so the pills are ~8 pt higher than in
  the mockups). The overlay computes the layout (`IOSTouch_PillGeometry`,
  `IOSTouch_GetHudLayout`); the game only reads it, so touch and HUD agree.
  Drawn by `ios/IOSHud.cpp` (`IOSHud_Draw`, from `R_RenderView` in
  `wl_draw.cpp` after the weapon) into ECWolf's 8-bit frame: composed in
  RGB, put back as the palette's nearest colours; the game's own graphics
  at run time (FONTN048.. digits, the % cut from STBACK, MEDIA0, ONUPA0,
  STKEYS1-4, the weapon's icon, the mugshot `gamestate.faceframe` keyed out
  of its panel), nothing in the repo. The HUD's strength is OPACITY's HUD
  slider (`iosTouchHudOpacity`, 20-100%): it fades the pills and their
  contents, and CLASSIC's status bar into the view's border
  (`IOSHud_StatusBarFadeBegin/End` around `WolfStatusBar::DrawStatusBar`).
  CAMERA: clean photos have no HUD (taken before the weapon), HUD photos
  have it. None in the photo views (selfie, freeze, cinema).
- **GAMES** (tray, only when more than one game is found; as the
  Half-Life and Raze ports): "Switch game", the picker's cards over the
  game (held still), "PLAYING" on the current one, Cancel
  (`IOSPicker_PresentSwitch`). The current one or Cancel closes it.
  Another: its name and the time kept (`wolf3dNextGameName/Time`),
  ECWolf's settings written, the app closes with a local notification
  "Tap to start <game>" (no permission: an alert says to open the app
  again); opening the app within 10 minutes starts it, no picker
  (`IOSPicker_TakeNext` in `I_PickIWad_Cocoa`). The games found come from
  `wl_iwad.cpp` (`IOSGame_SetGames`). Rig: `@TRAYGAMES` prints
  "rig: Switch game".
- **The cheat (`sithewok`):** the tray's CHEAT shows the CHEAT button (top
  row, half way between the middle and SAVE) and switches the cheat on, or
  tapped again leaves cheat mode (off, button gone). CHEAT held 0.5 s
  (gold ring) = on (gold, BJ's face in colour) / off (grey face); a tap does
  nothing. Its picture is BJ's status-bar face, grinning, from the player's
  own data at run time (`STFGOD0` in Spear, else `STFEVL0`, else `STFST00`;
  `FeedCheatIcon` in `IOSGame.cpp`; nothing in the repo). Coming on plays
  the game's "Yeah!" (`misc/yeah`); off is silent. Typing the word (key
  presses: the iOS keyboard sends letters as keys) toggles it and the
  button. While on (single player only; `KeepCheat`, once a tic): god mode,
  every weapon and the ammo (`IOSCheat_GiveAll` in `wl_debug.cpp`, the
  weapon in hand kept) and every key, given as it comes on and in each level
  started or loaded; ammo kept full. USE held still 0.5 s (gold ring) =
  noclip on/off, with a small red "HOLD: NOCLIP" / "NOCLIP ON" under USE;
  noclip ends at each new level. The tray's LEVEL (any single-player
  level, cheat or not; gold while the cheat is on) opens a list of the
  game's maps (MAP01..., `IOSGame_LevelInfo`: "Episode name, Floor n" in
  Wolf3D, the MAPINFO names in Spear, "(secret)" on secret floors; the
  episode/floor and map under), the game holding still; a tap warps there
  (ECWolf's `DEBUG_Warp`: the floor from its start as a new game, pistol,
  8 bullets, score 0, a new game's lives -- "Going to ...: a fresh start";
  with the cheat on, everything is given again). Off each launch; the button's showing is remembered
  (`iosTouchShowCheat`).
- **iOS code:** `ios/`: `i_ios.mm` (main, paths, the first screen, the
  picker's fallback and the error alert, the log, the 60 FPS cap; SDL's accelerometer joystick is off: ECWolf read
  it as a stick and the view turned by itself),
  `IOSPicker.mm`, `IOSTouch.m`/`.h` (the overlay, C core + UIKit view, from
  the UZDoom port's), `IOSGame.cpp` (the game side), `IOSHud.cpp` (the
  touch HUD), `IOSGameData.cpp`,
  `Info.plist.in`, `build_deps.sh` (SDL2, SDL2_mixer, SDL2_net). Engine
  changes are `#ifdef ECWOLF_IOS` (the app), `ECWOLF_TOUCH` (the touch
  controls: iOS and the rig) or `ECWOLF_IOSPATHS` (the data search): hooks
  in `id_in.cpp` (every event pump runs `IOSGame_Frame`), `wl_play.cpp`
  (the tic's command, the game held still by the wheel, quick save/load),
  `wl_loadsave.cpp` (`TouchQuickSave`/`TouchQuickLoad`: no question, the
  slot "Quicksave"), `sdlvideo.cpp`, `c_cvars.cpp` (iOS defaults: full
  screen, vsync, view size 20), `id_us_1.cpp` (the keyboard for typing a
  save's name), `r_sprites.cpp` (the wheel's pictures), `wl_main.cpp`, `wl_debug.cpp` (`IOSCheat_GiveAll`), `g_mapinfo.h`
  (`GetMapInfoName`, for the cheat's list), `wl_draw.cpp` (`IOSHud_Draw`),
  `g_wolf/wolf_sbar.cpp` (CLASSIC's fade).
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
  (points wide, high, scale, home indicator inset, and optionally the side
  insets, default 47; the overlay's layout follows it too) on an X screen
  of its pixels (`-screen 0 2622x1206x24`), no `--res`. The mockups'
  phone: `"956 440 3 21 62"` on `2868x1320x24`. With no game files it prints the first screen and
  exits 3. Script lines: `frame <n>`, `wait <n>`, `waitplay` (until a
  level is being played), touches (`down/move/up <id> <x y | @BUTTON>`,
  e.g. `@SAVE`, `@TRAYCHEAT`, `@TRAYHUD`; `tapslice`, `state`, `levels`,
  `level <i>`; `hud <split|strip|classic|none> [face]`, `opacityslide
  <buttons|hud> <0.2-1>` on the open OPACITY panel; the HUD's settings in
  `$HOME/.iosrig-hud`), `pos`, `key <SDL name>`, `shot <png>` (ImageMagick
  `import`), `rigset health n | ammo n | all | weapon i` (rig only: the
  HUD's warnings and contents), `quit`.
  `--tedlevel MAP01 --nowait` starts in a level. `key` presses and lets go
  in one go: the menus may miss it (use `@BACK` there). The rig draws no
  overlay: its shots show the game only. The picker is the
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
  - SAVE / LOAD (top right): tap = ECWolf's Save Game / Load Game menu (F2,
    F3); a 0.3 s hold with a ring = quick save / quick load. Sliding off
    cancels; SAVE wins a tie (LOAD's tap does nothing while SAVE is held);
    only a finished hold saves or loads.
  - MENU: tap = game menu; hold 0.45 s = tray with LOOK, ALWAYS RUN,
    OPACITY, HUD, FACE, FPS, CAMERA, CHEAT, LEVEL (in a level, single
    player), GAMES (more than one game found), keyboard, and the build
    stamp under it in small dim text (not a button; hidden while OPACITY's
    panel is open).
  - LOOK (tray): the drag's turning speed, shown on it ("LOOK 1x"); a tap
    steps 0.5, 0.75, 1, 1.25, 1.5, 2, 2.5, 3x (round to 0.5x); held 0.45 s
    (ring) back to 1x (build 2's speed) with a toast and a haptic.
    Remembered (`iosTouchLookScale`).
  - ALWAYS RUN (tray): ECWolf's own Always Run on/off (lit yellow while on;
    saved in its config at once). Off: a run marker (^^ in a small circle,
    100 pt above the stick, joined by a line; as the Jedi Knight port's)
    shows with the stick; the thumb slid up onto it runs straight ahead (a
    haptic tick, the knob on the marker, lit blue). On: no marker, the stick
    always runs.
  - OPACITY (tray): tap = a panel under the tray (`IOSTouch_OpacityPanel`)
    with two sliders, BUTTONS (the controls: `iosTouchOpacity`) and HUD (the
    game's HUD: `iosTouchHudOpacity`), each 20% to 100% in 5% steps, live,
    saved on lift, default 100%; hold = both back to 100% (toast, haptic).
    The tray and the wheel stay at full.
  - HUD (tray): tap = the next look (SPLIT, STRIP, CLASSIC, NONE; "HUD\n
    SPLIT"); FACE (tray): BJ's face in the pills on/off (lit while on, dim
    outside SPLIT and STRIP). See **HUD** above.
  - FPS (tray): tap = show/hide the FPS readout. Wolf3D differs from the
    other ports here (the owner's call): no 60/120 switch, no hold action,
    the label just "FPS"; the game runs at a fixed 60.
  - No GYRO in Wolf3D (the owner's call): no tilt aiming at all.
  - NEXT WPN: tap = next; a 10 pt slide opens the weapon wheel at once; a
    0.3 s hold opens it to tap. The gap at the bottom cancels. The game
    holds still while a wheel is open. A haptic tick per slice.
  - Wolf3D's buttons: FIRE and USE bottom right: in SPLIT 14 pt above the
    pills; in CLASSIC above the game's status bar (the game reports the
    bar's top, `IOSTouchHud.fStatusTop`, and a change lays them out again;
    no higher than the middle; nothing else sits on the bar); in STRIP and
    NONE above the home indicator; NEXT WPN and MAP (automap)
    top left; CAMERA in the top row's middle; CHEAT (cheat mode), SAVE,
    LOAD, MENU top right; the stick strafes sideways.
  - HUD and buttons stay clear of the corners, the notch or Dynamic Island,
    and the home bar.
- **Testing:** say plainly what was and wasn't tested. The owner tests on
  the device.
- **Lean mode:** small contexts, no full log or file dumps
  (grep/sed/head/tail), reuse builds.
- **Talking to the owner:** plain words, short messages, direct links, and
  honesty about mistakes and untested things. If they skip a question,
  choose a sensible default and say what you chose.
