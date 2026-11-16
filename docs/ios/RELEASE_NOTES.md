<!--
  The GitHub release text for a release run of .github/workflows/ios.yml: a
  manual run of branch ios with "release" set to a tag such as ios-v0.1. The
  workflow publishes this file, as it is in the commit the run builds, as the
  release body (body_path). Update it before each release: the run stops
  before building unless this file has a "### What's in <tag>" heading line
  for that tag (for example "### What's in ios-v0.1"), written exactly like
  that and on a line of its own. A '-' in the version after "ios-"
  (ios-v0.2-beta1) makes a prerelease; without one (ios-v0.1) the release is
  marked as the latest release. Only the .ipa files named in RELEASE_IPAS in
  ios.yml are attached (Wolf3D-ios.ipa). The list under "### What's in <tag>"
  also becomes the short text in the AltStore / SideStore source. Links must
  be absolute: relative links don't resolve on a release page. Keep each
  paragraph and list item on one line: a release page turns every line break
  into a visible one.
-->

**Wolf3D for iOS v0.1**: an unofficial iPhone and iPad port of [ECWolf](https://maniacsvault.net/ecwolf/), the Wolfenstein 3D source port, playing *Wolfenstein 3D*, *Spear of Destiny* and its two mission packs, with on-screen touch controls made for them.

**You need your own copy of the game; no game files are included.** Wolfenstein 3D on [GOG](https://www.gog.com/game/wolfenstein_3d) (it comes with Spear of Destiny) or [Steam](https://store.steampowered.com/app/2270/), and Spear of Destiny on [Steam](https://store.steampowered.com/app/9000/), all work. You copy the game's files onto your iPhone (see *Install*).

**Setup guide:** https://github.com/sithewok13-dev/ECWolf/blob/ios/docs/ios/README.md

**New here?** *Known limitations*, *Requirements* and *Install* are near the bottom of this page, just above the download.

### What's in ios-v0.1

The first numbered release of Wolf3D, the app. Everything in it:

- **Wolfenstein 3D and Spear of Destiny in one app**, from your own files. GOG's folders work just as they are, and with both games' files the tray's GAMES switches between them.
- **Full screen, widescreen, at a steady 60 frames a second**, in the game's own sharp pixels.
- **Touch controls:** a move stick wherever your left thumb lands (slide up onto the ^^ above it to run), drag anywhere else to turn, FIRE and USE under your right thumb, MAP for the automap.
- **A weapon wheel:** slide NEXT WPN, or hold it, and pick a weapon. The game waits while it's open.
- **Quick save and quick load:** hold SAVE or LOAD for 0.3 s. A tap opens the game's own Save Game or Load Game menu.
- **Six HUD looks:** BJ's face, health, lives, ammo and keys in two small pills (SPLIT) or one (STRIP), the original status bar whole (CLASSIC) or cut to fit the corners (CLASSIC SPLIT, CLASSIC STRIP), or nothing at all. Three sizes and an opacity slider.
- **A camera:** photos, bursts, selfies with BJ grinning in the corner and the enemies around you posing for the group shot, **Action Freeze** (time stops and you fly the camera around the moment) and a cinematic burst. Pictures go to your Photos.
- **The sithewok cheat:** a golden Ark of the Covenant button, with a sound made for it. God mode, every weapon, every key and full ammo; hold USE for noclip.
- **LEVEL:** a list of every level in the game you're playing. Tap one to go there.

---

### How to play: every control explained

The touch controls appear while you're playing and change in the menus, the title and story screens and between levels (there you tap and slide, see *Menus* below). The game runs in landscape, either way round.

Where everything is while you play (on a phone, turn it sideways to see the whole picture):

```
 NEXT  MAP             CAMERA      CHEAT        SAVE  LOAD  MENU
 WPN
   [LOOK] [ALWAYS RUN] [HUD] [FPS] [camera] [CHEAT] [LEVEL] [GAMES] [keyboard]
                        (MENU's tray, while it's open)

                                                        USE    FIRE
     ( o )  move stick: appears
            where your left thumb lands
 [face | health | lives]                           [weapon ammo | keys]
```

Exact positions shift a little to fit your screen, the camera cutout and the HUD. CHEAT only shows once you've switched the cheat on; LEVEL only in a level; GAMES only when the app has found more than one game.

#### Moving and turning

- **Move:** put your left thumb down anywhere in the left part of the screen. The stick appears right under your thumb and goes away when you lift it. Push up to go forward, down to back away, and sideways to step sideways (strafe).
- **Run:** while the stick is out, a small **^^** in a circle sits above it. Slide your thumb up onto it to run straight ahead (you feel a tick). Or hold MENU and switch on **ALWAYS RUN** in the tray: then the stick always runs and the ^^ goes away.
- **Turn:** drag anywhere else that isn't a button, which mostly means the right side. Dragging on FIRE or USE turns too, so you can hold FIRE and aim with the same thumb.
- **Turning speed:** hold MENU and tap **LOOK** in its tray. Each tap goes on to the next speed, **0.5x, 0.75x, 1x** (the default), **1.25x, 1.5x, 2x, 2.5x, 3x**, and after 3x round to 0.5x; the button shows the speed. **Hold LOOK** until its ring fills (0.45 s) to go back to 1x. The app remembers your speed.
- There's no tilt aiming in this port.

#### Buttons

- **FIRE** (bottom right): hold to keep firing.
- **USE** (next to FIRE): opens doors, flips the elevator switch and pushes secret walls.
- **NEXT WPN** (top left): a tap takes the next weapon. Slide or hold it for the weapon wheel (below).
- **MAP** (top left): the automap.
- **SAVE** and **LOAD** (top right): a tap opens the game's own Save Game or Load Game menu. Hold for quick save and quick load (below).
- **MENU** (top right): a tap opens the game's menu. Hold it for the tray (below).
- **CAMERA** (top, in the middle): photos and more (below).
- **RESUME** (in the middle): shows while the game is paused. Tap it to play on.

#### The weapon wheel

Slide your thumb off **NEXT WPN** (about 10 points) and the wheel opens at once; or hold NEXT WPN for 0.3 s and it opens for you to tap. It has one slice per weapon (knife, pistol, machine gun, chain gun), with its picture and ammo. Weapons you haven't found yet are faint. Slide onto a slice, or tap it, to take that weapon. The gap at the bottom cancels. The game waits while the wheel is open, and you feel a tick on each slice.

#### Quick save and quick load: why the short hold

Hold **SAVE** for 0.3 s (a ring fills) to save at once into the *Quicksave* slot, with no questions. Hold **LOAD** the same way to load it. The short hold is there so a thumb that brushes the corner in a fight doesn't throw away your progress. Sliding off the button before the ring fills cancels, and if you hold both, SAVE wins. A plain tap opens the game's own Save Game or Load Game menu, where you can name your saves (the iOS keyboard comes up).

#### MENU and its hidden tray

Hold **MENU** for 0.45 s and a tray opens under it:

- **LOOK:** turning speed (see *Moving and turning*).
- **ALWAYS RUN:** the game's own Always Run, on (lit) or off.
- **HUD:** opens the HUD panel (see *The HUD*).
- **FPS:** shows or hides a frame-rate counter. The game always runs at 60 frames a second.
- **The camera symbol:** shows or hides the CAMERA button. Hold it for the camera's options.
- **CHEAT:** see *The cheat* below.
- **LEVEL** (in a level): a list of the game's levels. The game holds still while it's open. Tap one to go there: you start that floor fresh, as a new game would, with the pistol and 8 bullets (with the cheat on, you get everything again). The X, or a touch off the list, closes it.
- **GAMES** (when the app found more than one game): *Switch game*, with a card for each game. Pick another and the app saves its settings and closes; tap its notification, or open it again within 10 minutes, and that game starts.
- **The keyboard symbol:** the iOS keyboard, for typing a cheat or anything else the game asks for.

Under the tray, in small letters, is the build, for example *build 23 · Nov 16*.

#### The camera: photos, selfies, bursts and Action Freeze

- **Tap CAMERA** for a photo. You'll see a flash and hear a shutter.
- **Hold CAMERA for 0.5 s** (a ring fills) for a **selfie**: BJ grins in a bottom corner, and the enemies in the area with you (not from elsewhere in the level) are posed in front of the camera, in a different arrangement each time. If no one is near, it says so.
- **Slide sideways** off CAMERA for a **burst** of 10 photos.
- **Slide down** for **Action Freeze**: time stops. Drag one finger to fly the camera around the moment; with two fingers, move it and pinch to zoom. The round white button takes the picture, and the X ends the freeze. In Wolfenstein 3D you see BJ running in the frozen scene; Spear of Destiny has no pictures of him, so he isn't there.
- **Slide up** for a **cinematic burst** of 5.
- **Options:** hold the camera symbol in MENU's tray: *PHOTO* (clean, with the HUD, or with the HUD and buttons), *SELFIES* (1, 3 or 5 per selfie) and *ACTION FREEZE* (on or off).
- A selfie or a freeze changes nothing in the game: everything is put back exactly as it was. Pictures go to your Photos app (the app only asks to add photos, never to see yours). If you don't allow that, they go to *Files > On My iPhone > Wolf3D > Screenshots*.

#### The cheat (sithewok)

- **Switch it on:** hold MENU, then tap **CHEAT** in the tray. The CHEAT button appears at the top, a golden **Ark of the Covenant**, opening with a sound made for it. Or type `sithewok` with the keyboard (the keyboard symbol in the tray).
- **While it's on:** god mode, every weapon, every key and full ammo, given again in each level you start or load.
- **Noclip (walk through walls):** hold **USE** still for 0.5 s (a gold ring fills). A small red *NOCLIP ON* shows under USE. Hold it again to switch it off. Noclip ends at each new level.
- **The CHEAT button:** hold it for 0.5 s to switch the cheat off (the Ark closes and turns grey) or on again. A tap does nothing, so it can't go off by accident.
- **Leaving cheat mode:** tap CHEAT in the tray again; the button goes away. The cheat always starts off when you open the app.

#### The HUD

Tap **HUD** in MENU's tray for its panel:

- **LOOK:** *SPLIT* (the default: two small pills at the bottom, BJ's face, health and lives on the left, the weapon, its ammo and the keys on the right), *STRIP* (the same in one pill in the middle), *CLASSIC SPLIT* and *CLASSIC STRIP* (the game's original status bar, cut to the corners or whole in the middle), *CLASSIC* (the original status bar under the view, as in 1992), or *NONE*.
- **SIZE:** small, medium or large.
- **FACE:** BJ's face in the pills, on or off.
- **HUD** and **BUTTONS:** how see-through the HUD and the touch buttons are, from 20% to 100%.
- Tap a row's name to reset that row. **Hold HUD** in the tray to put everything back to the defaults.
- Health under 25 turns red and glows; ammo turns red when it's low.

#### Menus, leaving the app

- In the menus, the title and story screens, and between levels: **tap** to choose or go on, **slide** up or down (or use the ▲ ▼ buttons on the right) to move, and **BACK** (top left) to go back. MENU works there too.
- **Leaving the app doesn't save your game.** Quick save first (hold SAVE).

---

### Known limitations

**Sorry about the bugs.** This is an unofficial beta, made by a fan with a lot of help from the open-source projects in the credits, and it will have bugs. Back up your saves now and then (see *Install*), and if something goes wrong, please tell us (see *Found a bug?*): every report helps.

- **No multiplayer, sorry.** Wolfenstein 3D is a single-player game. ECWolf has early multiplayer test code, but it can only be started from a computer's command line, so this app has none.
- **Limited testing so far:** the app has been played on one iPhone. Each feature was also tested on a computer, running the app's game code with the real game files and simulated touches. Not tried yet: iPads (the app is marked for them), iOS versions older than 18 (the app is built for iOS 15 and newer), game controllers, hardware keyboards and mice, and copies of the game other than GOG's.
- **The mission packs (Return to Danger, Ultimate Challenge) aren't confirmed yet.** The app finds them in GOG's `M2` and `M3` folders, but on the computer test setup they didn't show up as games to pick. If you have them and can't play them, please report it.
- **Installing from the source in SideStore or AltStore is new** and hasn't been tried yet. If it doesn't work for you, download `Wolf3D-ios.ipa` below and install it with your tool.
- **No tilt aiming,** and no 120 frames a second: the game always runs at 60.

### Requirements

- An **iPhone on iOS 15 or newer.** The app is also marked for iPad, but it hasn't been tried on one.
- **Your own copy of the game**, for its files:
  - **Wolfenstein 3D:** `AUDIOHED.WL6`, `AUDIOT.WL6`, `GAMEMAPS.WL6`, `MAPHEAD.WL6`, `VGADICT.WL6`, `VGAGRAPH.WL6`, `VGAHEAD.WL6` and `VSWAP.WL6`, about 2.5 MB.
  - **Spear of Destiny:** the same eight names ending in `.SOD`. GOG's copy keeps them in a folder called `m1` or `M1`.
  - **The mission packs**, if your copy has them: GOG's `M2` and `M3` folders as they are, or files ending in `.SD2` and `.SD3`.
- A **sideloading tool or signing service** (something that installs apps from outside the App Store), for example SideStore, AltStore (AltStore Classic, not AltStore PAL), Feather or Sideloadly. With a free Apple ID, apps installed this way stop opening after 7 days until you refresh them in the tool, and you can have at most 3 of them at once.

### Install

**With SideStore or AltStore (easiest):** set one of them up first, with its own instructions. Then, on your iPhone, tap **[Add to SideStore](https://sithewok13-dev.github.io/wolf3d/#add-sidestore)** or **[Add to AltStore](https://sithewok13-dev.github.io/wolf3d/#add-altstore)**. Or open it, go to *Sources*, tap **+** and paste this address: `https://raw.githubusercontent.com/sithewok13-dev/ECWolf/ios/packaging/ios/altstore-source.json`. Install **Wolf3D** from the source, then carry on from step 3 below. You update the app there too.

**Or with any sideloading tool or signing service:**

1. Download **`Wolf3D-ios.ipa`**, below.
2. Install it with your tool, following its own instructions. On your home screen the app is called **Wolf3D**. Its version shows as 0.1 followed by a build number.
3. **Open the app once.** With no game files yet, it shows *Game files needed*, saying where to copy them. That's expected: the app has now made its folder, *On My iPhone > Wolf3D*, in the Files app.
4. On your computer, open the folder the game is installed in. For GOG's Wolfenstein 3D, that's the folder with the `.WL6` files (and Spear of Destiny in its `m1` folder); for GOG's Spear of Destiny, the folder with `M1`, `M2` and `M3`. Not sure where it is? Search your computer for `VSWAP.WL6` or `VSWAP.SOD`.
5. Copy the files or whole folders into the Files app's **On My iPhone > Wolf3D**, with their names unchanged (not as a `.zip`). Folders inside folders are fine, up to two levels down. For example, copy them into iCloud Drive on your computer, then in the Files app on your iPhone move them from iCloud Drive into *On My iPhone > Wolf3D*.
6. **Go back to Wolf3D.** It starts by itself once it finds the files. If it found more than one game, tap the card for the one you want.

**Updating:** install the new version over the old one with the same tool and the same Apple ID, and your game files and saves stay.

**Your saves** and settings are in the Files app, in *On My iPhone > Wolf3D > ECWolf*. **Back up** that folder now and then (for example to iCloud Drive), and always before you delete or reinstall the app: deleting the app deletes its folder, game files and saves included. The LOOK, HUD and CAMERA settings are kept by iOS, not in that folder.

**If something goes wrong:** `wolf3d-log.txt`, in *On My iPhone > Wolf3D > ECWolf*, says what the game was doing. Copy it out before you open the app again, because each start replaces it.

**Download:** `Wolf3D-ios.ipa`, under *Assets* below. The *iOS latest build* prerelease, also on the Releases page, is rebuilt automatically after every code change, before anyone has tried it: use this release.

**Found a bug?** Please report it at https://github.com/sithewok13-dev/ECWolf/issues and not to the upstream ECWolf project, and say you're on **ios-v0.1** and which build: hold MENU and it's under the tray, for example *build 23 · Nov 16* (your sideloading tool shows the version as 0.1 and that build number). Attach `wolf3d-log.txt` (see *If something goes wrong*).

---

**Thank you** to everyone whose work this port stands on: **Braden "Blzut3" Obrzut** and the ECWolf contributors, for [ECWolf](https://maniacsvault.net/ecwolf/) ([source](https://github.com/ECWolfEngine/ECWolf)), years of careful work that keep Wolfenstein 3D playing well today; **Moritz "Ripper" Kroll**, for Wolf4SDL, which ECWolf grew from; **id Software** (John Carmack, John Romero, Adrian Carmack, Tom Hall and the team), for the games and for releasing their source code in 1995; **Randy Heit, Christoph Oelckers and Teemu Piippo**, whose ZDoom code ECWolf builds on; the **SDL** team (Sam Lantinga and contributors), for SDL, SDL_mixer and SDL_net; and the **DOSBox** and **MAME** teams, for the OPL sound emulators that play the music.

Unofficial build. ECWolf is free software: its code from the original games and Wolf4SDL is under the [GNU GPL v2 or later](https://github.com/sithewok13-dev/ECWolf/blob/ios/docs/license-gpl.txt) (or [id Software's original licence](https://github.com/sithewok13-dev/ECWolf/blob/ios/docs/license-id.txt)), its newer and ZDoom-derived code under the BSD 3-clause licence, and the parts listed in its [copyright file](https://github.com/sithewok13-dev/ECWolf/blob/ios/docs/copyright) under their own licences (the MAME sound emulator's [licence](https://github.com/sithewok13-dev/ECWolf/blob/ios/docs/license-mame.txt)). The iOS port's code, and the cheat's Ark pictures and sound made for it, are under the same terms. Source code: https://github.com/sithewok13-dev/ECWolf/tree/ios

Wolfenstein 3D and Spear of Destiny are trademarks of their owners. This port is not affiliated with or endorsed by id Software, ZeniMax, Bethesda or Microsoft, and it is not an official ECWolf release.
