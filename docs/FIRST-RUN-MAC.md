# First run on macOS (Apple Silicon) — DRAFT

**Status:** DRAFT, measured 2026-10-22/23 on build `base-5073d2e-3-g8ff361451`. It gets merged into `BUILD-MAC.md` in Fase 0.5.

## 1. Bring your own ROM

You need your own legally obtained **US** cartridge dump in **`.z64`** (big-endian) format. It must never go inside this repo.

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
```

The expected result is SHA-1 `579c48e211ae952530ffc8738709f078d5dd215e`, first bytes `8037 1240`, and 12,582,912 bytes. A `.n64` or `.v64` dump fails this check, and auto-detect skips it too.

## 2. Runtime directory and launch

```bash
RT=/Users/oneill/Sites/mk64-online/runtime/p1     # one directory per player instance (Fase 2 adds p2)
mkdir -p "$RT/rom" && ln -s /path/to/your.z64 "$RT/rom/baserom.us.z64"
cd "$RT/rom" && export SHIP_HOME="$RT"
/path/to/build-cmake/SpaghettiKart.app/Contents/MacOS/SpaghettiKart
```

Run the binary inside the bundle. **Do not** use `open SpaghettiKart.app`: `open` drops `SHIP_HOME` and the working directory, so the app falls back to `~/Library/Application Support/SpaghettiKart` and does not find the ROM. On macOS the app looks for the ROM by scanning the current directory for a `.z64` file, and it follows symlinks.

## 3. First launch: what you see

1. *"No O2R Files / No O2R files found. Generate one now?"*: answer **Yes**. Answering No exits the app.
2. *"Found Mario Kart 64 (US) / The extraction process will now begin."*: click **OK**. Extraction took about **16 s**.
3. The Spaghetti Kart logo intro plays, then the title screen. If you leave the title idle, attract-mode demo races play in a loop.

If a file dialog appears instead of "Found …", auto-detect failed: Cancel, fix the symlink and relaunch.

## 4. What the first run writes (`$SHIP_HOME`)

`mk64.o2r` is the extracted assets, a zip of about 26.7 MB with 32,447 entries. The other entries are `spaghettify.cfg.json`, `torch.hash.yml`, `default.sav`, `controllerPak_header.sav`, `imgui.ini`, `mods/` and `logs/Spaghetti Kart.log`, which rotates at 10 MB.

Side effect: an **empty** `~/Library/Application Support/p1` directory is created, named after the last path component of `SHIP_HOME`. It is harmless. `~/Library/Application Support/SpaghettiKart` is not created. The app bundle is not modified: `codesign --verify --deep --strict` still passes and the binary hash is unchanged.

The second launch shows no prompt, and `mk64.o2r` is byte-identical afterwards.

## 5. Controllers and 2-player split-screen

- **In-game menu: `Esc`**. (F1 toggles a menu bar, but on a Mac keyboard F1 is a brightness key.)
- Mapping: open Settings → Controls → **Popout Bindings Window** ("Input Editor"), then the Port 2 tab, and bind each button. **Close the editor before playing**: while it is open, all game input reads zero.
- **DualSense (PS5) over Bluetooth works with no `gamecontrollerdb.txt`.** Port 1 had the pad bound by default; Port 2 was bound by hand. Turn the pads on *before* launching.
- Verified: 2P VS, horizontal split-screen, both players driving a full race on two DualSense pads; zero `[error]` lines.

## 6. Known log lines (harmless)

- `[warning] Failed to add SDL game controller mappings from "./gamecontrollerdb.txt" (Invalid RWops)`: the controller DB is absent (see §5).
- `[critical] > Starting Torch… / Game / CRC / Hash`: the extractor's banner, logged at critical level. It is not an error.
- `[error] The extraction process will now begin.`: the dialog text, logged at error level.
- `[warning] CI8: null palette`: 3 lines per launch.
- `[error] RGBA Texture that isn't 16 or 32 bit. Size = 1`: appeared during attract-mode demos, about every 5 minutes. Not seen in player races. Not triaged.

## Not covered

Launching from Finder, Intel Macs, a second machine, and Windows.
