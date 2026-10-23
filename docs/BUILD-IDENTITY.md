# Build identity record (macOS)

`tools/build-identity.sh` prints a short `key=value` record that names exactly which build of SpaghettiKart you are running.
Two records can be compared with `diff`. This page explains why the record exists, what each field means, how to compare two
records, and every source of non-determinism found so far.

Measured on 2026-10-23 (UTC) at fork commit `8ff361451cd0a261314be72147285f419acc985d` (`base-5073d2e-3-g8ff361451`),
macOS 15.6.1 (24G90) arm64, Xcode 26.3, Apple clang 17.0.0.

## 1. Purpose

The MK64 Online design (design consequence #6) says:

> Build identity handshake = hash(game binary) + hash(`mk64.o2r`) + `gTickLogic` + FP-environment word + mod list.

This record is the **build half** of that handshake. The other three parts, `gTickLogic`, the FP-environment word and the mod list,
are runtime values. Later phases add them (Fase 1 and Fase 3).

The rule for a netplay session: if two peers disagree on any **MUST** field (§3), they do not start a session.

**The MUST on `game_binary_sha256` holds only when both peers run ONE distributed binary.** Two players who each build from
source with `BUILD-MAC.md` will get different binaries, even at the same commit. Two reasons were measured (§5):

- the binary contains absolute source paths, so a clone at another path gives another binary (probe b);
- two links of the same sources, on the same Mac and the same path, were not byte-identical (probe d).

So two self-built peers would never pass the handshake. That is a decision someone has to make (see §6, "Open decision").

## 2. How to produce it

Run it after the build has finished. While `ninja` is relinking, the binary hash would be torn. Run it after the game's first run
too (BUILD-MAC.md §7), because only the first run writes `mk64.o2r`; before that, add `--no-mk64`.

```bash
# The variables are BUILD-MAC.md §0's:
# FORK       = your clone of the fork (the directory that holds build-cmake/)
# SHIP_HOME  = the runtime directory the game wrote mk64.o2r into (BUILD-MAC.md §7)
# VCPKG_ROOT = the vcpkg clone you passed to cmake
# ID_DIR     = where the records go: a folder OUTSIDE every git repo, so a record is never committed by accident
ID_DIR="$MK64_ROOT/identity"
mkdir -p "$ID_DIR"
git -C "$ID_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1 && echo "STOP: $ID_DIR is inside a git repo - choose a folder outside any repo"
OUT="$ID_DIR/identity-$(git -C "$FORK" rev-parse --short HEAD)-$(date -u +%Y%m%dT%H%M%SZ).txt"
test -e "$OUT" && echo "STOP: $OUT already exists"
bash "$FORK/tools/build-identity.sh" \
  --fork-root "$FORK" \
  --app "$FORK/build-cmake/SpaghettiKart.app" \
  --ship-home "$SHIP_HOME" \
  --vcpkg-root "$VCPKG_ROOT" > "$OUT"
echo "rc=$?"
```

The file name carries the commit and the UTC time, so two records never overwrite each other and §4 can compare any two of them.

Keep the records out of git. `$MK64_ROOT/identity` is outside every repo only when `$MK64_ROOT` is a plain folder, as BUILD-MAC.md
§0 creates it. On the verified Mac `$MK64_ROOT` is itself a git repository in which `identity-*.txt` and `identity/` are not ignored
(`git check-ignore` exits 1 for both), so the check above prints `STOP:` there; use a scratch folder outside it instead. Measured on
2026-10-23: the check prints `STOP:` for a folder inside a new `git init` repository (also with a space in the path) and nothing
for a folder outside any repository.

`mk64_o2r_sha256` hashes whatever `mk64.o2r` is in `--ship-home`. BUILD-MAC.md's §0 block gives every clone on one Mac the same
`SHIP_HOME`, so that hash may come from another build's first run, and then it says nothing about this build. Measured on
2026-10-23: a second clone's record reported the first clone's `5c90905b…015956` although the second clone never ran the game.

Flags (all optional):

| flag | default |
|---|---|
| `--fork-root DIR` | the directory above `tools/` |
| `--app PATH` | `<fork-root>/build-cmake/SpaghettiKart.app` |
| `--ship-home DIR` | `$SHIP_HOME`, else `~/Library/Application Support/SpaghettiKart` (the app's own default, `src/port/Game.cpp:1050`). `HOME` is read only for this default, so with `--ship-home` (or `--no-mk64`) the script also runs with `HOME` unset |
| `--vcpkg-root DIR` | `$VCPKG_ROOT`, else `<build dir>/.vcpkg` (`cmake/automate-vcpkg.cmake:63`) |
| `--no-mk64` | off. Use it where there is no ROM, such as CI. `mk64_o2r_sha256` is then `not-checked` |

Exit codes:

| rc | meaning |
|---|---|
| `0` | record printed (17 lines) |
| `2` | a required input is missing, empty, or a directory. stderr names it, for example `MISSING: mk64.o2r (/nonexistent/mk64.o2r)`. `<fork-root>/vcpkg.json` is a required input (`MISSING: vcpkg.json (<path>)`). With `HOME` unset and neither `--ship-home` nor `$SHIP_HOME` nor `--no-mk64`, there is no ship home to look in: `MISSING: ship home (HOME is unset: pass --ship-home or set SHIP_HOME)` |
| `3` | a required input is unreadable (`UNREADABLE: …`) |
| `64` | usage error (unknown flag, or a flag without its value) |

On any non-zero exit, stdout is empty, so `> "$OUT"` never holds a half record: the file exists but is empty (measured: `rc=2`,
0 bytes, with an empty `--ship-home`, against the BUILD-MAC.md follower build at `3745d64dc`, 2026-10-23 17:04 UTC). All problems are
listed, not only the first.

## 3. Field table

MUST = peers that differ here do not start a session. SHOULD = a difference is a clue, not a block. info = never compared.

| key | meaning | source | peers |
|---|---|---|---|
| `fork_commit` | commit the fork checkout is at | `git rev-parse HEAD` | SHOULD |
| `fork_describe` | tag-relative name, `-dirty` if tracked edits | `git describe --tags --always --dirty` | SHOULD |
| `submodule_libultraship` | checked-out libultraship commit | `git -C libultraship rev-parse HEAD` | SHOULD |
| `submodule_torch` | checked-out torch commit | `git -C torch rev-parse HEAD` | SHOULD |
| `vcpkg_commit` | vcpkg tool clone commit (`absent` if none) | `git -C "$VCPKG_ROOT" rev-parse HEAD` | SHOULD |
| `vcpkg_baseline` | port baseline from the manifest | `vcpkg.json` `builtin-baseline` | SHOULD |
| `cmake_version` | first line of `cmake --version` | `cmake --version` | SHOULD |
| `ninja_version` | ninja version | `ninja --version` | SHOULD |
| `clang_version` | first line of `clang --version` | `clang --version` | SHOULD |
| `macos_version` | product version + build | `sw_vers` | SHOULD |
| `sdk_path` | active SDK path | `xcrun --show-sdk-path` | info |
| `game_binary_sha256` | the Mach-O inside the bundle, signature included | `shasum -a 256 …/Contents/MacOS/SpaghettiKart` | MUST — for one distributed binary only; self-built peers differ (open decision, §1) |
| `spaghetti_o2r_sha256` | port assets inside the bundle | `shasum -a 256 …/Contents/Resources/spaghetti.o2r` | MUST |
| `mk64_o2r_sha256` | archive extracted from the player's ROM (`not-checked` with `--no-mk64`) | `shasum -a 256 "$SHIP_HOME/mk64.o2r"` | MUST |
| `sse2neon_sha256` | the unpinned header compiled into the mixer | `shasum -a 256 build-cmake/_deps/sse2neon/sse2neon.h` | SHOULD |
| `config_yml_sha256` | ROM-hash → yamls map inside the bundle | `shasum -a 256 …/Contents/Resources/config.yml` | MUST |
| `generated_at` | UTC time the record was made | `date -u` | info |

A tool field (`cmake_version`, `ninja_version`, `clang_version`, `sdk_path`) prints `absent` when the tool is not on `PATH`, and
`unknown` when the tool runs but its first line of output is empty. No field is ever printed with an empty value.

A submodule directory that was never initialised prints `absent`. The script checks for `<dir>/.git` first, because
`git -C <empty submodule dir> rev-parse HEAD` silently returns the parent repo's commit.

This Mac's record, made 2026-10-23T16:13:40Z after the last rebuild:

```text
fork_commit=8ff361451cd0a261314be72147285f419acc985d
fork_describe=base-5073d2e-3-g8ff361451
submodule_libultraship=60c9bb713ed0850da66559fc17c556d03da649cd
submodule_torch=2d474ddb8da8b213fbdbb49d0273ce31fa955f35
vcpkg_commit=2e58bb35ff7a3a037920d959ce20cb4d8c22319a
vcpkg_baseline=2e58bb35ff7a3a037920d959ce20cb4d8c22319a
cmake_version=cmake version 4.3.4
ninja_version=1.13.2
clang_version=Apple clang version 17.0.0 (clang-1700.6.4.2)
macos_version=15.6.1 (24G90)
sdk_path=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
game_binary_sha256=91d3fc7bf6e753a01d724755a2eee18a0ee9188a2b715606a19635962839d998
spaghetti_o2r_sha256=b0d62f2032b8a632b2bc853c7d770c47a0939f797f496e0dd041d23ffdd5db67
mk64_o2r_sha256=5c90905bd7c340571304600a2babd6c7b38a46f0890830a7f50bbb20ab015956
sse2neon_sha256=932e41317cc325846256695d2aa83812ff6b6d065c8994b73a461efaf1f10bc3
config_yml_sha256=faee59ea12a3d02dfed1f8e98551f87d2ba31b8e5942c6f8ed67ba032f4adb87
generated_at=2026-10-23T16:13:40Z
```

## 4. Comparing two records

```bash
A="$ID_DIR/identity-<commit>-<time>.txt"   # the first record (§2 names them this way)
B="$ID_DIR/identity-<commit>-<time>.txt"   # the second one, or a record a peer sent you
diff <(grep -v '^generated_at=' "$A") <(grep -v '^generated_at=' "$B")
```

- **No output:** same build.
- **Only SHOULD lines differ:** the toolchains differ, but the build may still match. The MUST lines decide.
- **A MUST line differs while every SHOULD line matches:** walk §5 in order. The first source that applies is the likely cause.
- **`game_binary_sha256` differs between two self-built peers:** expected (§1). It is not a defect of either build.

Two runs of the script on the same unchanged build are identical apart from `generated_at` (measured 2026-10-23, two runs).

## 5. Known non-determinism sources

Each source has its evidence and its status as measured in Fase 0.5 (2026-10-23, commit `8ff361451`). VERIFIED means measured.
UNVERIFIED means not measured yet.

1. **`sse2neon.h` is downloaded unpinned, on every configure.** `cmake/dependencies/common.cmake:5` downloads it from the `master`
   branch of DLTcollab/sse2neon, with no `EXPECTED_HASH`. Its only consumer is the audio mixer on arm64
   (`src/audio/mixer.c:29`, under `#elif defined(__aarch64__)`), but it is still compiled into the binary.
   **VERIFIED (probe d):** a re-configure rewrote the file (mtime `1792724829` → `1792771984`). The bytes were unchanged on
   2026-10-23 (size 507039, sha256 `932e4131…10bc3`). The rewrite alone, with the same bytes, makes Ninja recompile and relink.
   The day upstream changes `master`, a fresh configure gets different code. `sse2neon_sha256` shows when that happened.
2. **`stb_image.h` is also re-downloaded on every configure** (found in probe d, not predicted). It comes from
   `libultraship/cmake/dependencies/common.cmake:55`, at a pinned stb commit, so its bytes should not change. Its rewrite still
   triggers a recompile. There is no field for it in the record.
3. **Relinking the same sources gives a different binary.** **VERIFIED (probe d):** after the re-configure, the rebuild ran
   19 compile/link lines and `game_binary_sha256` changed from `876387ec…73be9b6` to `91d3fc7b…9d998`. Same commit, same path, same
   Mac. With the signatures removed, both binaries are 13,052,336 bytes and 648 bytes differ: 16 bytes are `LC_UUID`
   (`C94EE9FB…` → `7E537DF3…`), the rest are in `__TEXT,__objc_stubs` (plus 1 byte in `__TEXT,__stubs`). There are no date or time
   strings. **The cause is UNVERIFIED** (linker or LTO ordering of the recompiled objects is a lead, not a finding).
4. **`spaghetti.o2r` carries build-time timestamps.** **VERIFIED (probe d):** the archive was rebuilt and
   `spaghetti_o2r_sha256` changed from `e7a25d58…a7e66` to `b0d62f20…db67`. Both archives have the same 369 entries, with identical
   names, sizes, compression methods and CRC-32s. Only the zip entry timestamps differ (every entry: 10-22-2026 22:08 vs
   10-23-2026 11:13, local time). The `GenerateO2R` step runs on every build; the copy into the bundle happens on relink.
   ⇒ **`spaghetti_o2r_sha256` is a MUST that does not reproduce across two builds of the same commit.** Until the archive is built
   with fixed timestamps, a peer comparison has to compare the entry CRCs instead of the file hash.
5. **vcpkg tool and ports.** The tool clone is pinned to `2e58bb35ff7a3a037920d959ce20cb4d8c22319a` by the fork (Fase 0.1), and the
   same commit is the port baseline in `vcpkg.json`. **VERIFIED:** `bootstrap-vcpkg.sh` downloads a separately versioned tool binary
   (`Downloading vcpkg-macos...`, 0.3's `configure.log:24`, giving `vcpkg package management program version
   2025-06-20-ef7c0d54…`). That binary is still pinned by the clone: its release tag (`VCPKG_TOOL_RELEASE_TAG=2025-06-20`) and its
   SHA-512 (`VCPKG_MACOS_SHA`) come from `scripts/vcpkg-tool-metadata.txt` in the pinned clone, and `scripts/bootstrap.sh:151`
   checks the hash. `vcpkg_commit` and `vcpkg_baseline` show both pins.
6. **Link-time optimisation.** Release builds use `-flto=auto` (`cmake/SetFlags.cmake:40,42`). **UNVERIFIED, deferred:** two clean
   builds have never been compared. That needs a second full build, which needs disk and an explicit OK.
7. **The ad-hoc signature covers the bundle resources.** **VERIFIED (probe c, on a scratch copy):** re-signing an unchanged copy
   twice kept `876387ec…` (ad-hoc signing is deterministic). Appending one byte to `Contents/Resources/config.yml` and re-signing gave
   `230eca8d10f44f9c3954af3d3b476f916f569fd0a36bfb30f8152c295c81fc60`. `codesign -dv` shows `Signature=adhoc` and
   `Sealed Resources … files=132`. ⇒ Changing any resource changes `game_binary_sha256`, even though the Mach-O code is the same.
   Upstream CI does exactly that: it copies its own `spaghetti.o2r` into the bundle and adds `gamecontrollerdb.txt` (downloaded from
   the `master` branch of SDL_GameControllerDB), then re-signs (`.github/workflows/main.yml:113-115`). A CI binary therefore differs
   from a local one unless those resources are byte-identical. (That the *added* file changes the hash is inferred from probe c,
   which tested a changed file; UNVERIFIED as such.)
8. **Absolute source paths are compiled in.** **VERIFIED (probe b):**
   `strings …/SpaghettiKart | grep -c '<fork path>/'` = 73 (40 under `libultraship/src`, 18 `torch/src`, 11 `src/port`,
   3 `src/engine`, 1 `src`). Control: an unrelated path = 0. No `-ffile-prefix-map` (or any `-f*-prefix-map`) exists in `cmake/`,
   `CMakeLists.txt` or `libultraship/cmake`. The paths come from `__FILE__` in log lines (`src/port/Engine.cpp:189` sets the
   `[%s:%#]` pattern). ⇒ A clone at another path gives another binary.
   Seen again on 2026-10-23 in the BUILD-MAC.md follower build (another path, commit `3745d64dc`): 73 strings with the follower's
   path, 0 with the local path, and `game_binary_sha256` `b29a0027…bb3a5a` instead of the local `91d3fc7b…9d998`. That build
   also differs by commit (item 9), so which item causes the difference was not separated.
9. **The version string is the `git describe` output.** `cmake/GetVersion.cmake:1-8` runs `git describe --tags`; the result becomes
   the compile definition `SPAGHETTI_VERSION` (`cmake/SetFlags.cmake:127`), logged at start-up by `src/port/Engine.cpp:192`
   (`Spaghetti Kart base-5073d2e-3-g8ff361451` in 0.4's log). A new commit or a new tag changes the binary even when the code is the
   same, and a fresh configure after a new commit recompiles everything. `GetVersion.cmake` does not pass `--dirty`, so uncommitted
   edits do not show in the version string; they do show in the record's `fork_describe`.
10. **Xcode, SDK and macOS version.** Different compilers or SDKs are expected to give different binaries. `clang_version`,
    `sdk_path` and `macos_version` record them. Not measured across machines.

## 6. Not yet covered

- **Mac vs Windows:** the records differ by construction (another binary, another toolchain, no `sse2neon`). Fase 6.
- **`mk64.o2r` across machines:** it is per player, extracted from that player's ROM. It should match across machines only when both
  ROMs have SHA-1 `579c48e211ae952530ffc8738709f078d5dd215e`, and even that is UNVERIFIED until two machines extract.
- **Runtime identity:** `gTickLogic`, the FP-environment word and the mod list are not in this record yet (Fase 1 and Fase 3).

**Open decision** (for the owner of the Fase 3 handshake and the spec; Tarek decides): either **ship one `.app` to both players**, or
**make the build path-independent** (for example `-ffile-prefix-map`). Until then, the `game_binary_sha256` MUST only holds for one
distributed binary. Note that path independence alone would not be enough: two links on the same path also differed (§5 item 3).
