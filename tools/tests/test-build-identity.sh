#!/bin/bash
# test-build-identity.sh — tests tools/build-identity.sh against a fake tree (no real build, no ROM).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUT="$HERE/../build-identity.sh"
fail() { echo "FAIL: $*" >&2; exit 1; }
[ -f "$SUT" ] || fail "script under test not found: $SUT"
T="$(mktemp -d "${TMPDIR:-/tmp}/bi-test.XXXXXX")"
trap 'chmod -R u+rwX "$T" 2>/dev/null; rm -rf "$T"' EXIT
G() { git -c user.name=t -c user.email=t@example.invalid -C "$@"; }
mkrepo() { mkdir -p "$1"; G "$1" init -q; G "$1" commit -q --allow-empty -m init; }

F="$T/fork"; mkdir -p "$F"
printf 'build-cmake/\nlibultraship/\ntorch/\n' > "$F/.gitignore"
printf '{\n  "builtin-baseline": "2e58bb35ff7a3a037920d959ce20cb4d8c22319a"\n}\n' > "$F/vcpkg.json"
G "$F" init -q; G "$F" add .; G "$F" commit -q -m base; G "$F" tag -a base-test -m base
mkrepo "$F/libultraship"; mkrepo "$F/torch"; mkrepo "$T/vcpkg"
APP="$F/build-cmake/SpaghettiKart.app"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$F/build-cmake/_deps/sse2neon"
printf 'BINARY-1' > "$APP/Contents/MacOS/SpaghettiKart"
printf 'PORT-O2R' > "$APP/Contents/Resources/spaghetti.o2r"
printf 'cfg: 1\n'  > "$APP/Contents/Resources/config.yml"
printf '/* fake */\n' > "$F/build-cmake/_deps/sse2neon/sse2neon.h"
S="$T/ship home"; mkdir -p "$S"; printf 'GAME-O2R' > "$S/mk64.o2r"   # space on purpose (real default has one)

run() { bash "$SUT" --fork-root "$F" --app "$APP" --ship-home "$S" --vcpkg-root "$T/vcpkg" "$@"; }
nots() { grep -v '^generated_at=' "$1"; }
KEYS="fork_commit fork_describe submodule_libultraship submodule_torch vcpkg_commit vcpkg_baseline cmake_version
ninja_version clang_version macos_version sdk_path game_binary_sha256 spaghetti_o2r_sha256 mk64_o2r_sha256
sse2neon_sha256 config_yml_sha256 generated_at"

# (1) two runs identical after dropping generated_at; exact key set, in order
run > "$T/a.txt"; run > "$T/b.txt"
diff <(nots "$T/a.txt") <(nots "$T/b.txt") >/dev/null || fail "(1) two runs differ"
[ "$(cut -d= -f1 "$T/a.txt" | tr '\n' ' ')" = "$(echo $KEYS) " ] || fail "(1) key list/order wrong: $(cut -d= -f1 "$T/a.txt" | tr '\n' ' ')"
grep -qx 'vcpkg_baseline=2e58bb35ff7a3a037920d959ce20cb4d8c22319a' "$T/a.txt" || fail "(1) baseline not parsed"
grep -qx 'fork_describe=base-test' "$T/a.txt" || fail "(1) describe wrong"

# (2) one flipped byte in the binary changes game_binary_sha256 and nothing else
printf 'BINARY-2' > "$APP/Contents/MacOS/SpaghettiKart"
run > "$T/c.txt"
changed="$( { diff <(nots "$T/a.txt") <(nots "$T/c.txt") || true; } | sed -n 's/^> \([a-z0-9_]*\)=.*/\1/p')"  # diff exits 1
[ "$changed" = "game_binary_sha256" ] || fail "(2) expected only game_binary_sha256 to change, got: '$changed'"

# (3) missing mk64.o2r -> exit 2, names it, prints NO partial record on stdout
rm "$S/mk64.o2r"
set +e; run > "$T/d.out" 2> "$T/d.err"; rc=$?; set -e
[ "$rc" -eq 2 ] || fail "(3) rc=$rc, expected 2"
grep -q 'MISSING: mk64.o2r' "$T/d.err" || fail "(3) no 'MISSING: mk64.o2r' on stderr"
[ ! -s "$T/d.out" ] || fail "(3) partial record printed on failure"

# (4) --no-mk64 (CI has no ROM) -> exit 0, explicit marker
run --no-mk64 > "$T/e.txt" || fail "(4) --no-mk64 failed"
grep -qx 'mk64_o2r_sha256=not-checked' "$T/e.txt" || fail "(4) marker missing"

# (5) unreadable input -> exit 3 naming it (permission denied)
chmod 000 "$F/build-cmake/_deps/sse2neon/sse2neon.h"
set +e; run --no-mk64 > /dev/null 2> "$T/f.err"; rc=$?; set -e
[ "$rc" -eq 3 ] || fail "(5) rc=$rc, expected 3"
grep -q 'UNREADABLE: sse2neon.h' "$T/f.err" || fail "(5) not named"
echo "PASS: build-identity (5 cases)"
