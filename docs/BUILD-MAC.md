# Building and running on macOS (Apple Silicon)

This page takes you from nothing to a playable `SpaghettiKart.app` and a 2-player split-screen race, on an Apple Silicon Mac.
It is the MK64 Online fork's macOS guide. Upstream's `docs/BUILDING.md` is left as it is, but its macOS section is out of date
(see the note in §5).

## 1. Status

**Verified on 2026-10-23 (UTC) at fork commit `8ff361451cd0a261314be72147285f419acc985d` (`base-5073d2e-3-g8ff361451`) on macOS
15.6.1 (24G90) arm64.** Unless a row says otherwise, every number on this page was measured on that Mac, at that commit, and
carries its date. Nothing is estimated. What was not tested is listed in §11.

A second, context-free reader then followed §0 to §6 on the same Mac (2026-10-23), in a new clone at another path, at commit
`3745d64dc` (`base-5073d2e-7-g3745d64dc`, the commit that added this page). Its numbers are in §9, marked **follower**. It did
not do §7 or §8 (they need a ROM, pads and a person at the screen); those were done once, on the verified build.

**Which binary hash is the reference.** The local build at `8ff361451` is now
`91d3fc7bf6e753a01d724755a2eee18a0ee9188a2b715606a19635962839d998`, the value in `docs/BUILD-IDENTITY.md` §3. It was first
`876387ec85abb8c01ebcfc49471f29a9f9cbae71705991aaf9a6a451073be9b6`: that binary is the one the first run and the 2-player races
(§7, §8) were played with. A later re-configure relinked it and changed the hash (BUILD-IDENTITY.md §5 item 3). **Your build will
not match either one.** A clone at another path, or at another commit, gives another binary (BUILD-IDENTITY.md §5 items 8 and 9).
The follower build got `b29a0027528ee77c1a06341c0843e7cb68c5c9b2d13e4e1c9f4f76fe76bb3a5a`.

## 0. Before you start: the shell and the variables

Every command on this page is written for **bash**. macOS Terminal starts zsh, so first type `bash` on its own and press Enter.

Then set these variables. Every section uses them, and a new Terminal window does not keep them, so paste this block again in each
new window (after `bash`). The values below are the layout of the verified Mac, with your home folder in front. You can pick other
paths, as long as `FORK` does not exist yet and none of them is inside the folder that holds your ROM.

```bash
export MK64_ROOT="$HOME/Sites/mk64-online"             # one parent folder for everything below
export FORK="$MK64_ROOT/SpaghettiKart"                 # the clone of the fork (created in §3)
export VCPKG_ROOT="$MK64_ROOT/vcpkg"                   # vcpkg clone; configure creates it if it is missing (§5)
export VCPKG_DEFAULT_BINARY_CACHE="$MK64_ROOT/vcpkg-cache"   # vcpkg's binary cache, kept next to the rest
export SHIP_HOME="$MK64_ROOT/runtime/p1"               # the game's writable folder for player 1 (§7)
export ROM="/path/to/your/mario-kart-64-us.z64"        # YOUR ROM file, as an ABSOLUTE path (starts with /); replace the placeholder (§7)
mkdir -p "$MK64_ROOT" "$VCPKG_DEFAULT_BINARY_CACHE"
```

Paths can contain spaces (the game's own default does). Keep the double quotes in every command.

`ROM` must be an absolute path. §7.2 links to it, and a link to a relative path points somewhere else once you `cd`: `ln -s` still
succeeds and the link is silently broken.

`SHIP_HOME` is not tied to one clone. A second clone on the same Mac that uses this same block shares it: the same `mk64.o2r`,
saves and controller bindings. For a separate one, give that clone another `SHIP_HOME` (for example `runtime/p2`).

## 2. Prerequisites

| What | Measured on the verified Mac | Check command |
|---|---|---|
| Apple Silicon | `arm64` | `uname -m` |
| macOS | `ProductVersion: 15.6.1`, `BuildVersion: 24G90` | `sw_vers` |
| Xcode (full app, selected) | `Xcode 26.3 Build version 17C529`; `/Applications/Xcode.app/Contents/Developer` | `xcodebuild -version`; `xcode-select -p` |
| Apple clang (comes with Xcode) | `Apple clang version 17.0.0 (clang-1700.6.4.2)` | `clang --version` |
| git | `git version 2.50.1 (Apple Git-155)` | `git --version` |
| Homebrew (to install cmake and ninja) | installed (cmake and ninja resolved from `/opt/homebrew/bin`) | `brew --version` |
| CMake | `cmake version 4.3.4` | `cmake --version` |
| Ninja | `1.13.2` | `ninja --version` |
| Free disk | 26 GiB free at the start; the build used about 1.7 GB in total (§9) | `df -g "$MK64_ROOT"` |
| Network | `github.com` and `raw.githubusercontent.com` reachable over HTTPS | §4 checks both |
| A Mario Kart 64 **US** ROM, `.z64`, that you own | SHA-1 `579c48e211ae952530ffc8738709f078d5dd215e` | §7 |
| For 2 players: two game controllers | two DualSense (PS5) pads over Bluetooth | §8 |

Install what is missing:

```bash
xcode-select -p || echo "install Xcode from the App Store, then: sudo xcode-select -s /Applications/Xcode.app"
xcodebuild -checkFirstLaunchStatus || sudo xcodebuild -runFirstLaunch   # agrees to the licence, installs Xcode's extra components
command -v brew || echo "install Homebrew first: follow https://brew.sh, then open a new Terminal and come back to §0"
brew install cmake ninja          # skip the ones `command -v cmake ninja` already finds
```

A new Xcode needs its licence accepted and its first-launch components installed before `clang` and `xcodebuild` work.
`xcodebuild -checkFirstLaunchStatus` exits non-zero when that is still to do, and `sudo xcodebuild -runFirstLaunch` does it
(`man xcodebuild`: *"Install additional system components and agree to the license."*). Opening Xcode once does the same. On the
verified Mac the check returned 0, so neither `-runFirstLaunch` nor the Homebrew installer was run there.

The build also uses these tools, which come with macOS and Xcode: `xcodebuild`, `xcode-select`, `sips`, `iconutil`, `plutil`,
`codesign`, `install_name_tool`, `ditto`, `shasum`, `otool`, `curl`. §4 checks all of them.

The ROM check in §7.1 also uses `xxd` and `stat`. §4 does **not** check those two. Both come with macOS, in `/usr/bin`
(`command -v xxd stat`).

Only Apple Silicon (arm64) was built and tested.

## 3. Clone

Clone the `netplay` branch directly, with its submodules. Do not clone the default branch and change branch afterwards: the clone
would set up the submodules of `main` (a different commit), and changing branch does not move them, so `submodule status` would show
`+` marks.

```bash
test -e "$FORK" && echo "STOP: $FORK already exists - choose another FORK"
git clone --branch netplay --recurse-submodules https://github.com/toneill57/SpaghettiKart.git "$FORK"
git -C "$FORK" submodule update --init --recursive      # safe to repeat; repairs a clone made without --branch
git -C "$FORK" submodule status
git -C "$FORK" describe --tags
test -f "$FORK/tools/build-identity.sh" && test -f "$FORK/docs/BUILD-IDENTITY.md" && echo identity-tool-ok
```

Expected: `submodule status` prints these four lines. Each starts with a space (no `+` or `-`), and each ends with the
submodule's own tag description in parentheses:

```text
 9760c30014131f4eacb8e96f15f3869c7bc5dd8c doxygen-awesome-css (v2.3.4-3-g9760c30)
 60c9bb713ed0850da66559fc17c556d03da649cd libultraship (1.3.1-479-g60c9bb71)
 d89593c8ccde9ef511e96ec1f0169988d60fc0a1 tools/blender/fast64 (v2.2.1-64-gd89593c)
 2d474ddb8da8b213fbdbb49d0273ce31fa955f35 torch (v1.0.0-356-g2d474dd)
```

`describe --tags` starts with `base-5073d2e` (on the verified commit it printed `base-5073d2e-3-g8ff361451`; a later commit of
`netplay` prints a higher count and another hash).

The last line should print `identity-tool-ok`. This page, `docs/BUILD-IDENTITY.md` and `tools/build-identity.sh` reach
`netplay` together, in the merge that adds this page. If the line prints nothing, your `netplay` is older than this page:
§4 to §8 still work unchanged (they were verified at `8ff361451`, which has no copy of this page), but skip §7.6 (the identity
record) until `netplay` has the tool.

## 4. Pre-flight check

```bash
bash "$FORK/tools/preflight-mac.sh"; echo "rc=$?"
```

Expected: the last line is `PREFLIGHT: OK` and `rc=0`. It prints the host, every tool and its version, the free disk space and the
network check. On failure it exits `2` and prints **every** problem, each on a line starting with `MISSING:` (a tool) or `DISK:`.

The script's default disk floor is 30 GiB. That is a cautious allowance, not a measurement: the build actually used about 1.7 GB
(§9). So a `DISK:` failure does not by itself mean the build will not fit.

**Only if the one problem printed is a `DISK:` line**, and `df -g "$MK64_ROOT"` shows at least 5 GiB free, run it again with a
floor of 5:

```bash
PREFLIGHT_MIN_FREE_GIB=5 bash "$FORK/tools/preflight-mac.sh"; echo "rc=$?"
```

The override changes the disk check only. A `MISSING:` line still has to be fixed. 5 GiB is about three times the measured
1.7 GB; nothing below 5 was tested. Measured on 2026-10-23: the verified Mac (26 GiB free) and the follower (24 GiB free) both
failed the default floor with a single `DISK:` line (`PREFLIGHT: FAIL`; the follower also printed `rc=2`) and passed with
`PREFLIGHT_MIN_FREE_GIB=20`. A floor of 5 passed (`PREFLIGHT: OK`, `rc=0`) on the same Mac with 23 GiB free, on 2026-10-23
at 17:03 UTC, with the follower clone's `tools/preflight-mac.sh` (clone at `3745d64dc`; the script last changed in `c53068e22`).

## 5. Configure and build

```bash
cd "$FORK"
cmake -H. -Bbuild-cmake -GNinja -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DENABLE_VCPKG=ON -DVCPKG_ROOT="$VCPKG_ROOT"
cmake --build build-cmake --config Release -j3
```

These are upstream CI's two lines (`.github/workflows/main.yml:101-102`) plus `-DVCPKG_ROOT`, so that every build directory
shares one vcpkg clone.

What the configure prints, and what it means. Not every line appears every time: the "When" column says when it does.

| Line | When | Meaning |
|---|---|---|
| `Spaghetti Kart version: base-5073d2e-… Alfa` | always | the version string from `git describe`, then a NATO word for the patch number (`cmake/GetVersion.cmake:34-48`; it is `Alfa` for every `base-…` describe). The follower printed `base-5073d2e-7-g3745d64dc Alfa` |
| `-- Cloning vcpkg in …` then `-- Pinning vcpkg in … to 2e58bb35ff7a3a037920d959ce20cb4d8c22319a` | only when `VCPKG_ROOT` does not exist yet | the fork clones vcpkg and pins it |
| `-- vcpkg already at pinned commit 2e58bb35ff7a3a037920d959ce20cb4d8c22319a` | when `VCPKG_ROOT` is already at the pin | both the verified Mac and the follower printed this |
| `Bootstrapping vcpkg in …` / `Downloading vcpkg-macos...` | only when vcpkg's tool binary `$VCPKG_ROOT/vcpkg` is missing: the first configure, or after the pin moved (`cmake/automate-vcpkg.cmake:169,199-201`) | vcpkg downloads its tool binary (version `2025-06-20-ef7c0d54…`). The verified Mac printed it; the follower, which reused that tool binary, did not |
| `Restored N package(s) from <your VCPKG_DEFAULT_BINARY_CACHE> in …` | always | `N` packages came from the binary cache. With an empty cache `N` is 0 and vcpkg builds the 19 ports (verified Mac: `Restored 0 package(s)`). With a warm cache it skips the builds (follower: `Restored 19 package(s) … in 134 ms`) |
| `error: \`vcpkg install\` requires a list of packages to install in classic mode.` | always | **harmless**: a second, empty install call fails; its exit code is ignored and configure continues. It is followed by vcpkg's usage text for `install` (`Synopsis:`, `Examples:`, `Options:` and the option list, about 50 lines), which is just as harmless |
| `--   VCPKG_ROOT.....: <your VCPKG_ROOT>` | always | the vcpkg clone in use |
| `-- Configuring done (…s)` / `-- Build files have been written to: <FORK>/build-cmake` | always | configure succeeded |

The verified Mac had the vcpkg clone ready before its first configure, so it printed the "already at pinned commit" line. The
"Cloning" path is covered by the fork's own CMake test (`vcpkg_pin_test`, all 15 checks passing), not by a real build on this page.

Ninja numbers the build steps out of 567, but the last counter it prints is **`[566/567]`** (the link-and-bundle step); there is
no `[567/567]` line, in the verified build or in the follower build. The build prints about 2,600 compiler `warning:` lines from
upstream code (2585 in both builds); they are expected. Success is the last two lines: `-- fixup_bundle: done` and
`…/build-cmake/SpaghettiKart.app: replacing existing signature`.

> **Stale upstream path.** Upstream `docs/BUILDING.md:215` says to run `./build/Spaghettify`. That path is stale. The product is
> `build-cmake/SpaghettiKart.app`. (`Spaghettify` is still the CMake target name, `CMakeLists.txt:5`.)

## 6. Expected result

```bash
cd "$FORK"
test -x build-cmake/SpaghettiKart.app/Contents/MacOS/SpaghettiKart && echo binary-ok
ls build-cmake/SpaghettiKart.app/Contents/Resources
codesign --verify --deep --strict build-cmake/SpaghettiKart.app; echo "codesign rc=$?"
```

Expected: `binary-ok`; `Resources` holds `Assets.car`, `config.yml`, `meta`, `spaghetti.o2r`, `SpaghettiKart.icns` and `yamls`;
`codesign rc=0`. The app is ad-hoc signed (`codesign -dv` shows `Signature=adhoc`) and links only system libraries
(`otool -L` lists nothing from Homebrew).

The bundle has **no** `gamecontrollerdb.txt`. Upstream CI adds one; a local build does not, and DualSense pads work without it (§8).

To record exactly which build you have, wait until the first run is done (§7.6): the record also hashes `mk64.o2r`, which only
the first run writes.

<a id="first-run"></a>

## 7. First run

### 7.1 Check your ROM

You need your own legally obtained **US** cartridge dump in **`.z64`** (big-endian) format. Never put it inside the repo.

```bash
check_rom() {  # exit 0 only for the US .z64 ROM the app accepts
  local f="$1"
  [ -n "$f" ] || { echo "FAIL: ROM path empty"; return 2; }
  [ -r "$f" ] || { echo "FAIL: not readable: $f"; return 2; }
  local sha; sha=$(shasum -a 1 "$f" | cut -d' ' -f1)
  [ "$sha" = "579c48e211ae952530ffc8738709f078d5dd215e" ] || { echo "FAIL: sha1=$sha"; return 3; }
  xxd -l 4 "$f" | grep -q '8037 1240' || { echo "FAIL: byte order: $(xxd -l 4 "$f")"; return 4; }
  echo "OK sha1=$sha size=$(stat -f %z "$f")"
}
check_rom "$ROM"; echo "rc=$?"
```

Expected: `OK sha1=579c48e211ae952530ffc8738709f078d5dd215e size=12582912` and `rc=0`. A `.n64` or `.v64` dump has the wrong byte
order: it fails this check, and the game's auto-detect skips it too. A `.zip` must be unzipped first.

### 7.2 Prepare the runtime folder and launch

```bash
case "$ROM" in /*) ;; *) echo "STOP: ROM must be an absolute path, got: $ROM (§0)" ;; esac
mkdir -p "$SHIP_HOME/rom"
[ -L "$SHIP_HOME/rom/baserom.us.z64" ] || ln -s "$ROM" "$SHIP_HOME/rom/baserom.us.z64"   # a link, not a copy
cd "$SHIP_HOME/rom"
"$FORK/build-cmake/SpaghettiKart.app/Contents/MacOS/SpaghettiKart"
```

- Run the binary **inside** the bundle, from Terminal, in the folder that holds the ROM link. On macOS the game looks for the ROM
  by scanning the **current folder** for a `.z64` file (`src/port/GameExtractor.cpp:117-171`), and it follows links. Keep only one
  `.z64` there.
- `SHIP_HOME` must be exported in that same shell. It is where the game writes everything (§7.4).
- **Do not** use `open SpaghettiKart.app` or double-click it in Finder. That launch does not carry your `SHIP_HOME` or your
  current folder, so the game uses its own default, `~/Library/Application Support/SpaghettiKart` (`src/port/Game.cpp:1050`), and
  will not find the ROM in the folder above. (Read from the code; a Finder launch was not run, see §11.)

### 7.3 What you see the first time

1. *"No O2R Files / No O2R files found. Generate one now?"*: answer **Yes**. Answering No exits the app.
2. *"Found Mario Kart 64 (US) / The extraction process will now begin."*: click **OK**. Extraction took about **16 s**.
3. The Spaghetti Kart logo intro plays, then the title screen. Left idle, the title plays attract-mode demo races in a loop.

If a file dialog appears instead of "Found …", auto-detect did not find the ROM: click Cancel, fix the link (§7.2) and launch again.

The second launch shows no prompt and goes straight to the logo intro and the title. `mk64.o2r` is byte-identical afterwards.

### 7.4 What the first run writes

All of it goes into `$SHIP_HOME`:

| Entry | What it is |
|---|---|
| `mk64.o2r` | the extracted game assets: a zip of 26,664,858 bytes with 32,447 entries |
| `spaghettify.cfg.json` | settings, including controller bindings |
| `torch.hash.yml`, `default.sav`, `controllerPak_header.sav`, `imgui.ini` | extractor state, save data, window layout |
| `mods/` | empty folder for mods |
| `logs/Spaghetti Kart.log` | the log; it rotates at 10 MB into `Spaghetti Kart.1.log`, `.2.log` |

```bash
ls -l "$SHIP_HOME/mk64.o2r"
```

One side effect outside it: an **empty** folder `~/Library/Application Support/<last part of SHIP_HOME>` is created (with the
variables of §0, `~/Library/Application Support/p1`). It is harmless. `~/Library/Application Support/SpaghettiKart` is not created.
The app bundle is not modified: `codesign --verify --deep --strict` still passes and the binary's hash is unchanged.

### 7.5 Log lines you can ignore

All of these appeared on the verified Mac; none of them broke anything.

- `[warning] Failed to add SDL game controller mappings from "./gamecontrollerdb.txt" (Invalid RWops)`: the controller database is
  absent (see §6). Pads still work.
- `[critical] > Starting Torch…`, `> Game…`, `> CRC…`, `> Hash…` and separator lines: the extractor's banner, logged at critical level.
  It is not an error.
- `[error] The extraction process will now begin.`: the dialog text, logged at error level.
- `[warning] CI8: null palette`: 3 lines per launch.
- `[error] RGBA Texture that isn't 16 or 32 bit. Size = 1`: seen only during attract-mode demos, about every 5 minutes, never in a
  player race. Not investigated.
- `[error] Failed to process models/tracks/<track>/…_packed_dl_<addr>`: 17 lines during extraction, one per affected track display
  list. **Not explained yet.** Koopa Troopa Beach has one, and it raced with no visible defect.

In the player sessions, the log had zero `[error]` and zero `[critical]` lines.

### 7.6 Record which build you have

After the first run, `$SHIP_HOME/mk64.o2r` exists and the identity record can be made (skip this if §3's last check printed
nothing). Keep the records out of git: write them to a folder that is not inside any git repository (not `$FORK`, and not
`$MK64_ROOT` if you made that a repository), under a name that says which commit and when:

```bash
ID_DIR="$MK64_ROOT/identity"   # any folder outside every git repo; the check below stops if it is inside one
mkdir -p "$ID_DIR"
git -C "$ID_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1 && echo "STOP: $ID_DIR is inside a git repo - choose a folder outside any repo"
OUT="$ID_DIR/identity-$(git -C "$FORK" rev-parse --short HEAD)-$(date -u +%Y%m%dT%H%M%SZ).txt"
test -e "$OUT" && echo "STOP: $OUT already exists"
bash "$FORK/tools/build-identity.sh" --fork-root "$FORK" --app "$FORK/build-cmake/SpaghettiKart.app" \
  --ship-home "$SHIP_HOME" --vcpkg-root "$VCPKG_ROOT" > "$OUT"; echo "rc=$?"
```

Expected: no `STOP:` line, `rc=0`, and `$OUT` holds 17 `key=value` lines. On the verified Mac `$MK64_ROOT` is itself a git
repository in which `identity/` is not ignored (`git check-ignore` exits 1), so the check prints `STOP:` there and the record has to
go to another folder, such as a scratch folder outside it. `docs/BUILD-IDENTITY.md` explains every field, how to compare two
records (§4) and why two builds differ (§5).

- **Before the first run** there is no `mk64.o2r`. The tool then exits `2` with `MISSING: mk64.o2r (…)` and `$OUT` is empty.
  Add `--no-mk64` to make the record anyway: `mk64_o2r_sha256` then reads `not-checked`. (Both measured on 2026-10-23 at 17:04 UTC
  against the follower build at `3745d64dc`, with an empty folder as `--ship-home`, using the script as of `3745d64dc` and as of
  `d807d6232`: without `--no-mk64` `rc=2` and 0 bytes on stdout, with it `rc=0` and 17 lines.)
- **`mk64_o2r_sha256` hashes whatever is in `$SHIP_HOME`.** With the §0 block, every clone on this Mac uses the same `SHIP_HOME`
  (§0), so the hash can come from another build's first run and then proves nothing about this build. The follower's record
  showed exactly that: it reported the local build's `5c90905b…015956`, and the follower never extracted one itself.

## 8. Two-player split-screen

### 8.1 Controllers

**Turn both pads on before you launch.** Pair them first in System Settings → Bluetooth. Two DualSense (PS5) pads over Bluetooth
were tested, and they work with no `gamecontrollerdb.txt`.

The game's default bindings (from `src/port/Engine.cpp:99-151`), as N64 button → pad / keyboard:

| N64 | Pad (SDL standard layout; DualSense name) | Keyboard (port 1) |
|---|---|---|
| A (accelerate, confirm) | bottom face button (Cross) | Shift |
| B (brake, back) | left face button (Square) | Control |
| Start | Start (Options) | Enter |
| Z (item) | left trigger (L2) | Z |
| R (hop/drift) | right trigger (R2) or right shoulder (R1) | Space |
| L | left shoulder (L1) | Q |
| Stick | left stick | arrow keys |

On the verified Mac, pad 1 was bound to **Port 1 by default**. Pad 2 had to be bound to **Port 2 by hand**:

1. Press **`Esc`** to open the in-game menu. (F1 toggles a menu bar instead; on a Mac keyboard F1 is a brightness key unless you
   hold `fn`.) `Esc` again closes it.
2. In the menu, open **Settings → Controls**, then click **Popout Bindings Window**. A window titled **Input Editor** opens.
3. Click the **Port 2** tab.
4. In each row (under **Buttons**, **D-Pad** and **Analog Stick**) click the **+** button. The slot says *"Press any button, move any
   axis, or press any key to add mapping"*. Press that button, or move the left stick in that direction, **on pad 2**. Do every
   button and all four stick directions. Port 1's tab shows what each row should get. (The verified Port 2 ended up with the same
   layout as Port 1.)
5. **Close the Input Editor before playing.** While it is open, all game input reads zero.

The bindings are saved in `$SHIP_HOME/spaghettify.cfg.json`, so this is needed only once per `SHIP_HOME`.

### 8.2 Menu path to a 2-player VS race

In these menus the stick or the D-pad moves the cursor, A (or Start) confirms and B goes back. Either pad can drive them; on the
character screen each player uses their own pad.

1. **Title screen:** press A or Start.
2. **Player count** (the row of 1P / 2P / 3P / 4P): push **right** until **2P** is highlighted, press A.
3. **Mode** (for 2P the list is MARIO GP, VS, BATTLE, top to bottom): push **down** until **VS** is highlighted, press A.
4. **Class** (50cc / 100cc / 150cc): pick one with up/down, press A.
5. **OK, still on the same mode screen:** picking the class does not open a new screen. The same menu now offers **OK** (the
   game's "OK option after selecting a submode", `src/menus.h:100`, handled by `main_menu_act` like steps 2-4). Press A on either
   pad.
6. **Character select:** each player moves their own cursor with their own pad and presses A. When both have chosen, the screen
   offers **OK**; press A on either pad (`player_select_menu_act` reads A from every pad in that state).
7. **Course select:** pick the cup with left/right (the first is **Mushroom Cup**) and press A; pick the course with up/down and
   press A; press A on **OK**. Mushroom Cup's courses are, in order: Luigi Raceway, Moo Moo Farm, Koopa Troopa Beach, Kalimari
   Desert.

The race screen is split into two viewports, one above the other.

**Verified on 2026-10-23** (build `base-5073d2e-3-g8ff361451`, two DualSense pads): a 2P VS race on Koopa Troopa Beach, both players
driving, about 108 s (player 1 on lap 2/3 at the last screenshot), and a 2P Grand Prix on Luigi Raceway for about 46 s. Zero
`[error]` lines in both sessions. The menu path above is written from the game's menu code (`src/menus.c`, `main_menu_act`,
`player_select_menu_act`, `course_select_menu_act`) and matches what was played; the button presses themselves were not logged.

## 9. Measured cost

All values are from one Mac (macOS 15.6.1 arm64, 14 CPU cores, 36 GiB RAM). They are not all from one moment: the "When" column
says, per row, whether the value comes from the first configure and build (vcpkg binary cache newly created and empty), from just
after it (03:11), from a later step on the same build directory, or from the game's first run.

| Step | Value | When | Date (UTC) | Commit |
|---|---|---|---|---|
| Configure (first, incl. vcpkg bootstrap + 19 ports) | 169.5 s | first configure, empty binary cache | 2026-10-23 03:05 | `8ff361451` |
| Build, `-j3` (Ninja total 567; last counter shown `[566/567]`) | 172.8 s | first build | 2026-10-23 03:08 | `8ff361451` |
| No-op rebuild (nothing changed) | 1.03 s (the asset packing step `GenerateO2R` always re-runs; nothing compiles) | just after the first build | 2026-10-23 03:11 | `8ff361451` |
| Re-configure of an existing build | 6.2 s, no ports rebuilt (the next build then runs 19 compile/link steps, see BUILD-IDENTITY.md §5) | later, binary cache already filled by the first configure | 2026-10-23 16:13 | `8ff361451` |
| `du -sh build-cmake` | 762 MB | just after the first build | 2026-10-23 03:11 | `8ff361451` |
| `du -sh "$VCPKG_ROOT"` | 863 MB (of which `buildtrees` 477 MB, `downloads` 152 MB) | just after the first build | 2026-10-23 03:11 | `8ff361451` |
| `du -sh build-cmake/vcpkg_installed` (measured on its own) | 43 MB | later, after the re-configure above | 2026-10-23 16:21 | `8ff361451` |
| `du -sh "$VCPKG_DEFAULT_BINARY_CACHE"` | 11 MB | just after the first build | 2026-10-23 03:11 | `8ff361451` |
| Total on disk | about 1.7 GB; free space went from 26 to 25 GiB | first build, measured just after it | 2026-10-23 03:11 | `8ff361451` |
| First-run extraction | about 16 s | the game's first run | 2026-10-23 03:14 | `8ff361451` |
| `mk64.o2r` | 26,664,858 bytes, 32,447 entries | the game's first run | 2026-10-23 03:14 | `8ff361451` |

**Follower build** (same Mac, a new clone at another path, the vcpkg clone and binary cache already warm from the build above):

| Step | Value | Date (UTC) | Commit |
|---|---|---|---|
| Clone + submodules (§3) | 16 s (from a local clone of the fork; submodules from GitHub) | 2026-10-23 16:30 | `3745d64dc` |
| Configure (warm: `Restored 19 package(s)`, no bootstrap) | 55.6 s (`Configuring done`), 56 s wall clock | 2026-10-23 16:31 | `3745d64dc` |
| Build, `-j3` (last counter `[566/567]`, 2585 `warning:` lines) | 174 s wall clock, `rc=0` | 2026-10-23 16:35 | `3745d64dc` |
| §6 checks | `binary-ok`; the six `Resources` entries; `codesign rc=0`; `Signature=adhoc`; `otool -L` lists 0 Homebrew libraries; no `gamecontrollerdb.txt` | 2026-10-23 16:35 | `3745d64dc` |
| `du -sh build-cmake` | 762 MB | 2026-10-23 16:36 | `3745d64dc` |
| Free disk | 24 GiB before configure, 23 GiB after the build | 2026-10-23 16:36 | `3745d64dc` |
| `game_binary_sha256` | `b29a0027528ee77c1a06341c0843e7cb68c5c9b2d13e4e1c9f4f76fe76bb3a5a`. Not comparable to the local build's: this binary embeds another `git describe` (`base-5073d2e-7-g3745d64dc`, BUILD-IDENTITY.md §5 item 9) as well as another source path (item 8) | 2026-10-23 16:35 | `3745d64dc` |
| `spaghetti_o2r_sha256` | `c84a0d22d566e3c4756ad051f9dfae9f4f767b43cf6406d9d4041720b311a291` (differs too; BUILD-IDENTITY.md §5 item 4 is one known cause, not tested here) | 2026-10-23 16:35 | `3745d64dc` |

Apart from `fork_commit`, `fork_describe` and the two hashes above, its identity record matched the local one
(`identity-local.txt`, binary `91d3fc7b…`) line for line: submodules, vcpkg commit and baseline, toolchain, macOS, SDK,
`sse2neon_sha256` and `config_yml_sha256`. Its `mk64_o2r_sha256` matched too, but only
because the follower shared the local `SHIP_HOME` (§7.6). The follower binary holds 73 strings with the follower's path and 0 with
the local path (BUILD-IDENTITY.md §5 item 8). Which of the known causes makes its binary differ was not tested.

`du` counts a hard-linked file only once per command, so measure `build-cmake/vcpkg_installed` on its own. On 2026-10-23 at
16:21 UTC (verified build, `8ff361451`), one `du` call together with `build-cmake` showed it as 96 KB, because 495 of its 517 files
were hard links, while on its own it was 43 MB. Re-checked at 17:03 UTC the same day: in the verified build none of the 517 files
was a hard link any more and both ways gave 43 MB; in the follower build (`3745d64dc`) 495 of 517 were, and the one-call `du` again
gave 96 KB. What removed the links in the verified build was not measured.

vcpkg packages installed (19, triplet `arm64-osx`), 2026-10-23 at `8ff361451`:

| Package | Version | Package | Version |
|---|---|---|---|
| bzip2 | 1.0.8#6 | nlohmann-json | 3.12.0 |
| egl-registry | 2024-01-25 | opengl | 2022-12-04#3 |
| fmt | 10.2.1#2 | opengl-registry | 2024-02-10#1 |
| glew | 2.2.0#6 | sdl2 | 2.32.8 |
| glfw3 | 3.4#1 | **sdl2-net** | 2.2.0#3 — installed but **not linked**: no CMake file links it and no source file uses it |
| libogg | 1.3.6 | spdlog | 1.14.1 |
| libpng | 1.6.48 | tinyxml2 | 11.0.0 |
| libvorbis | 1.3.7#4 | vcpkg-cmake | 2024-04-23 |
| libzip | 1.11.4 | vcpkg-cmake-config | 2024-05-23 |
| | | zlib | 1.3.1 |

`fmt` 10.2.1 and `spdlog` 1.14.1 are pinned on purpose by `vcpkg.json` (its `overrides`, lines 34-45).

## 10. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Configure stops inside a dependency with a CMake error about compatibility with old CMake versions (not observed: the verified build always passed the flag) | `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` was left out; CMake 4 refuses old `cmake_minimum_required` lines | Re-run the §5 configure line exactly as written |
| Compile errors in `fmt` mentioning `consteval` or format strings (not observed) | `vcpkg.json`'s `overrides` (lines 34-45, pinning fmt 10.2.1 and spdlog 1.14.1) were changed | `git -C "$FORK" diff -- vcpkg.json`, restore the file, configure again |
| `DISK:` from pre-flight, or "No space left on device" | not enough free space | Free space; the build needs about 1.7 GB (§9). Keep `VCPKG_DEFAULT_BINARY_CACHE` set so vcpkg reuses built packages |
| `error: \`vcpkg install\` requires a list of packages …` | a second, empty install call (harmless, see §5) | Nothing; configure still succeeds |
| vcpkg bootstrap fails, or prints `Expected hash` (not observed) | the tool download (`vcpkg-macos`) failed or was corrupted; bootstrap checks its SHA-512 | Check the network (§4), configure again |
| Configure keeps using an old vcpkg path after you changed `VCPKG_ROOT` | `build-cmake/CMakeCache.txt` remembers the first one | `mv "$FORK/build-cmake/CMakeCache.txt" "$FORK/build-cmake/CMakeCache.txt.old"`, then configure again |
| A file dialog asks you to select a ROM | the current folder had no `.z64`, or the link is broken | §7.2: `cd "$SHIP_HOME/rom"` and check `ls -lL baserom.us.z64` |
| The game cannot find the ROM, or settings seem lost | launched with `open` or from Finder, so `SHIP_HOME` was not used | Launch from Terminal as in §7.2 |
| Player 2's pad does nothing | Port 2 has no bindings, or the Input Editor is still open | §8.1 steps 2-5 |
| Two build identity records differ | see the list of known causes | `docs/BUILD-IDENTITY.md` §4 and §5 |
| `build-identity.sh` exits `2` with `MISSING: mk64.o2r (…)` | the first run has not written `$SHIP_HOME/mk64.o2r` yet | Do §7 first, or add `--no-mk64` (§7.6) |
| Pre-flight prints only `DISK: … below PREFLIGHT_MIN_FREE_GIB=30` | the default floor is a cautious 30 GiB | §4: with at least 5 GiB free, run it again with `PREFLIGHT_MIN_FREE_GIB=5` |

## 11. Not covered

These were not built or tested, and this page does not describe them:

- The Homebrew / MacPorts dependency path in upstream `docs/BUILDING.md:188` (`-DENABLE_VCPKG=OFF`). CI does not build it either.
- Debug builds (`-DCMAKE_BUILD_TYPE=Debug`). In the code, Debug adds `-g` (`cmake/SetFlags.cmake:78`) and, with Apple clang,
  turns LTO off with `-fno-lto` (`cmake/SetFlags.cmake:104-111`). Only Release was built.
- Building with only the Xcode Command Line Tools instead of the full Xcode app.
- Intel Macs.
- Windows and Linux.
- Launching from Finder (§7.2 describes it from the code only).
- One pad plus the keyboard for 2 players, and 3 or 4 players.
- Packaging (`cpack`, a DMG), notarization, and sending the app to another Mac (Gatekeeper quarantine). Later phases (Fase 5 and 6).
- A second build on another Mac. (A second build at another path on the same Mac **was** made: the follower in §9, at
  `3745d64dc`, build `rc=0` and `codesign rc=0`. Its binary hash differs from the local one, but it also embeds another
  `git describe` (`base-5073d2e-7-g3745d64dc`), so the two hashes are not comparable and the effect of the path alone was not
  separated; see `docs/BUILD-IDENTITY.md` §5 items 8 and 9.)
