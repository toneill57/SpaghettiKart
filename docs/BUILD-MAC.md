# Building and running on macOS (Apple Silicon)

This page takes you from nothing to a playable `SpaghettiKart.app` and a 2-player split-screen race, on an Apple Silicon Mac.
It is the MK64 Online fork's macOS guide. Upstream's `docs/BUILDING.md` is left as it is, but its macOS section is out of date
(see the note in §5).

## 1. Status

**Verified on 2026-10-23 (UTC) at fork commit `8ff361451cd0a261314be72147285f419acc985d` (`base-5073d2e-3-g8ff361451`) on macOS
15.6.1 (24G90) arm64.** Every number on this page was measured on that Mac, at that commit, and carries its date. Nothing is
estimated. What was not tested is listed in §11.

## Before you start: the shell and the variables

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
export ROM="/path/to/your/mario-kart-64-us.z64"        # YOUR ROM file; replace the placeholder (§7)
mkdir -p "$MK64_ROOT" "$VCPKG_DEFAULT_BINARY_CACHE"
```

Paths can contain spaces (the game's own default does). Keep the double quotes in every command.

## 2. Prerequisites

| What | Measured on the verified Mac | Check command |
|---|---|---|
| Apple Silicon | `arm64` | `uname -m` |
| macOS | `15.6.1 (24G90)` | `sw_vers -productVersion` |
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
xcode-select -p || echo "install Xcode from the App Store, open it once, then: sudo xcode-select -s /Applications/Xcode.app"
brew install cmake ninja          # skip the ones `command -v cmake ninja` already finds
```

The build also uses these tools, which come with macOS and Xcode: `xcodebuild`, `xcode-select`, `sips`, `iconutil`, `plutil`,
`codesign`, `install_name_tool`, `ditto`, `shasum`, `otool`, `curl`. §4 checks all of them.

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
```

Expected: `submodule status` lists these four commits, each line starting with a space (no `+` or `-`):

```text
 9760c30014131f4eacb8e96f15f3869c7bc5dd8c doxygen-awesome-css
 60c9bb713ed0850da66559fc17c556d03da649cd libultraship
 d89593c8ccde9ef511e96ec1f0169988d60fc0a1 tools/blender/fast64
 2d474ddb8da8b213fbdbb49d0273ce31fa955f35 torch
```

`describe --tags` starts with `base-5073d2e` (on the verified commit it printed `base-5073d2e-3-g8ff361451`; a later commit of
`netplay` prints a higher count and another hash).

## 4. Pre-flight check

```bash
bash "$FORK/tools/preflight-mac.sh"; echo "rc=$?"
```

Expected: the last line is `PREFLIGHT: OK` and `rc=0`. It prints the host, every tool and its version, the free disk space and the
network check. On failure it exits `2` and prints **every** problem, each on a line starting with `MISSING:` (a tool) or `DISK:`.

The script's default disk floor is 30 GiB. The verified Mac had 26 GiB free and passed with a floor of 20:

```bash
PREFLIGHT_MIN_FREE_GIB=20 bash "$FORK/tools/preflight-mac.sh"; echo "rc=$?"
```

The 30 GiB is a cautious allowance, not a measurement: the build actually used about 1.7 GB (§9).

## 5. Configure and build

```bash
cd "$FORK"
cmake -H. -Bbuild-cmake -GNinja -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DENABLE_VCPKG=ON -DVCPKG_ROOT="$VCPKG_ROOT"
cmake --build build-cmake --config Release -j3
```

These are upstream CI's two lines (`.github/workflows/main.yml:101-102`) plus `-DVCPKG_ROOT`, so that every build directory
shares one vcpkg clone.

What the configure prints, and what it means:

| Line | Meaning |
|---|---|
| `Spaghetti Kart version: base-5073d2e-…` | the version string, taken from `git describe` |
| `-- Cloning vcpkg in …` then `-- Pinning vcpkg in … to 2e58bb35ff7a3a037920d959ce20cb4d8c22319a` | first configure with an empty `VCPKG_ROOT`: the fork clones vcpkg and pins it |
| `-- vcpkg already at pinned commit 2e58bb35ff7a3a037920d959ce20cb4d8c22319a` | `VCPKG_ROOT` was already at the pin (this is what the verified Mac printed) |
| `Bootstrapping vcpkg in …` / `Downloading vcpkg-macos...` | vcpkg downloads its tool binary (version `2025-06-20-ef7c0d54…`) |
| `error: \`vcpkg install\` requires a list of packages to install in classic mode.` | **harmless**: a second, empty install call fails; its exit code is ignored and configure continues |
| `--   VCPKG_ROOT.....: <your VCPKG_ROOT>` | the vcpkg clone in use |
| `-- Configuring done (…s)` / `-- Build files have been written to: <FORK>/build-cmake` | configure succeeded |

The verified Mac had the vcpkg clone ready before its first configure, so it printed the "already at pinned commit" line. The
"Cloning" path is covered by the fork's own CMake test (`vcpkg_pin_test`, all 15 checks passing), not by a real build on this page.

The build runs 567 steps and prints about 2,600 compiler `warning:` lines from upstream code; they are expected. It ends with
`-- fixup_bundle: done` and `…/build-cmake/SpaghettiKart.app: replacing existing signature`.

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

To record exactly which build you have, see `docs/BUILD-IDENTITY.md`.

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
5. **OK:** press A.
6. **Character select:** each player moves their own cursor with their own pad and presses A. When both have chosen, press A on
   **OK**.
7. **Course select:** pick the cup with left/right (the first is **Mushroom Cup**) and press A; pick the course with up/down and
   press A; press A on **OK**. Mushroom Cup's courses are, in order: Luigi Raceway, Moo Moo Farm, Koopa Troopa Beach, Kalimari
   Desert.

The race screen is split into two viewports, one above the other.

**Verified on 2026-10-23** (build `base-5073d2e-3-g8ff361451`, two DualSense pads): a 2P VS race on Koopa Troopa Beach, both players
driving, about 108 s (player 1 on lap 2/3 at the last screenshot), and a 2P Grand Prix on Luigi Raceway for about 46 s. Zero
`[error]` lines in both sessions. The menu path above is written from the game's menu code (`src/menus.c`, `main_menu_act`,
`player_select_menu_act`, `course_select_menu_act`) and matches what was played; the button presses themselves were not logged.

## 9. Measured cost

All values from one Mac (macOS 15.6.1 arm64, 14 CPU cores, 36 GiB RAM), a first build with a newly created, empty vcpkg binary
cache.

| Step | Value | Date (UTC) | Commit |
|---|---|---|---|
| Configure (first, incl. vcpkg bootstrap + 19 ports) | 169.5 s | 2026-10-23 03:05 | `8ff361451` |
| Build, `-j3` (567 steps) | 172.8 s | 2026-10-23 03:08 | `8ff361451` |
| No-op rebuild (nothing changed) | 1.03 s (the asset packing step `GenerateO2R` always re-runs; nothing compiles) | 2026-10-23 03:11 | `8ff361451` |
| Re-configure of an existing build | 6.2 s, no ports rebuilt (the next build then runs 19 compile/link steps, see BUILD-IDENTITY.md §5) | 2026-10-23 16:13 | `8ff361451` |
| `du -sh build-cmake` | 762 MB | 2026-10-23 03:11 | `8ff361451` |
| `du -sh "$VCPKG_ROOT"` | 863 MB (of which `buildtrees` 477 MB, `downloads` 152 MB) | 2026-10-23 03:11 | `8ff361451` |
| `du -sh build-cmake/vcpkg_installed` (measured on its own) | 43 MB | 2026-10-23 16:21 | `8ff361451` |
| `du -sh "$VCPKG_DEFAULT_BINARY_CACHE"` | 11 MB | 2026-10-23 03:11 | `8ff361451` |
| Total on disk | about 1.7 GB; free space went from 26 to 25 GiB | 2026-10-23 03:11 | `8ff361451` |
| First-run extraction | about 16 s | 2026-10-23 03:14 | `8ff361451` |
| `mk64.o2r` | 26,664,858 bytes, 32,447 entries | 2026-10-23 03:14 | `8ff361451` |

`du` counts a hard-linked file only once per command, so measure `build-cmake/vcpkg_installed` on its own: in one `du` call
together with `build-cmake` it showed 96 KB, because 495 of its 517 files are hard links.

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

## 11. Not covered

These were not built or tested, and this page does not describe them:

- The Homebrew / MacPorts dependency path in upstream `docs/BUILDING.md:188` (`-DENABLE_VCPKG=OFF`). CI does not build it either.
- Debug builds (`-DCMAKE_BUILD_TYPE=Debug`, which uses `-O0` and turns LTO off, `cmake/SetFlags.cmake:106-110`). Only Release was built.
- Building with only the Xcode Command Line Tools instead of the full Xcode app.
- Intel Macs.
- Windows and Linux.
- Launching from Finder (§7.2 describes it from the code only).
- One pad plus the keyboard for 2 players, and 3 or 4 players.
- Packaging (`cpack`, a DMG), notarization, and sending the app to another Mac (Gatekeeper quarantine). Later phases (Fase 5 and 6).
- A second build on another Mac or at another path. It is expected to produce a different binary hash: see `docs/BUILD-IDENTITY.md`.
