#!/bin/bash
# test-build-identity.sh — tests tools/build-identity.sh against a fake tree (no real build, no ROM).
# Every case runs and reports; the exit code is 1 if any check failed. Field VALUES are compared with what the
# fixture really holds (shasum of the exact file, git of the exact repo), not only the key set.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUT="$HERE/../build-identity.sh"
FAILS=0
die()  { echo "ABORT: $*" >&2; exit 1; }                       # fixture/harness broken: nothing below is meaningful
fail() { echo "FAIL: $*" >&2; FAILS=$((FAILS + 1)); return 0; }
[ -f "$SUT" ] || die "script under test not found: $SUT"
T="$(mktemp -d "${TMPDIR:-/tmp}/bi-test.XXXXXX")"
trap 'chmod -R u+rwX "$T" 2>/dev/null; rm -rf "$T"' EXIT
G() { git -c user.name=t -c user.email=t@example.invalid -C "$@"; }
mkrepo() { mkdir -p "$1"; G "$1" init -q; G "$1" commit -q --allow-empty -m "init $2"; }   # distinct message => distinct commit
sha() { shasum -a 256 "$1" | awk '{print $1}'; }
field() { sed -n "s/^$1=//p" "$2"; }
expect() {  # expect <label> <file> <key> <value>
  local got; got="$(field "$3" "$2")"
  [ "$got" = "$4" ] || fail "$1 $3='$got', expected '$4'"
}

F="$T/fork"; mkdir -p "$F"
printf 'build-cmake/\nlibultraship/\ntorch/\n' > "$F/.gitignore"
printf '{\n  "builtin-baseline": "2e58bb35ff7a3a037920d959ce20cb4d8c22319a"\n}\n' > "$F/vcpkg.json"
printf 'readme\n' > "$F/README"
G "$F" init -q; G "$F" add .; G "$F" commit -q -m base; G "$F" tag -a base-test -m base
mkrepo "$F/libultraship" lus; mkrepo "$F/torch" torch; mkrepo "$T/vcpkg" vcpkg
APP="$F/build-cmake/SpaghettiKart.app"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$F/build-cmake/_deps/sse2neon"
printf 'BINARY-1' > "$APP/Contents/MacOS/SpaghettiKart"
printf 'PORT-O2R' > "$APP/Contents/Resources/spaghetti.o2r"
printf 'cfg: 1\n'  > "$APP/Contents/Resources/config.yml"
printf '/* fake */\n' > "$F/build-cmake/_deps/sse2neon/sse2neon.h"
S="$T/ship home"; mkdir -p "$S"; printf 'GAME-O2R' > "$S/mk64.o2r"   # space on purpose (real default has one)
BIN="$APP/Contents/MacOS/SpaghettiKart"; PO2R="$APP/Contents/Resources/spaghetti.o2r"
CFG="$APP/Contents/Resources/config.yml"; SSE="$F/build-cmake/_deps/sse2neon/sse2neon.h"; MK="$S/mk64.o2r"

# Positive control on the fixture itself: if two inputs were identical, a field reading the wrong one would still pass.
[ "$(printf '%s\n' "$(sha "$BIN")" "$(sha "$PO2R")" "$(sha "$CFG")" "$(sha "$SSE")" "$(sha "$MK")" | sort -u | wc -l | tr -d ' ')" = 5 ] \
  || die "fixture files are not pairwise distinct"
[ "$(printf '%s\n' "$(G "$F" rev-parse HEAD)" "$(G "$F/libultraship" rev-parse HEAD)" "$(G "$F/torch" rev-parse HEAD)" \
  "$(G "$T/vcpkg" rev-parse HEAD)" | sort -u | wc -l | tr -d ' ')" = 4 ] || die "fixture repos do not have 4 distinct commits"

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

# (6) every hash field = shasum of the exact file it names; every commit field = git of the exact repo
expect "(6)" "$T/a.txt" game_binary_sha256   "$(sha "$BIN")"
expect "(6)" "$T/a.txt" spaghetti_o2r_sha256 "$(sha "$PO2R")"
expect "(6)" "$T/a.txt" mk64_o2r_sha256      "$(sha "$MK")"
expect "(6)" "$T/a.txt" sse2neon_sha256      "$(sha "$SSE")"
expect "(6)" "$T/a.txt" config_yml_sha256    "$(sha "$CFG")"
expect "(6)" "$T/a.txt" fork_commit            "$(G "$F" rev-parse HEAD)"
expect "(6)" "$T/a.txt" fork_describe          "$(G "$F" describe --tags --always --dirty)"
expect "(6)" "$T/a.txt" submodule_libultraship "$(G "$F/libultraship" rev-parse HEAD)"
expect "(6)" "$T/a.txt" submodule_torch        "$(G "$F/torch" rev-parse HEAD)"
expect "(6)" "$T/a.txt" vcpkg_commit           "$(G "$T/vcpkg" rev-parse HEAD)"

# (2) one flipped byte in the binary changes game_binary_sha256 and nothing else
printf 'BINARY-2' > "$BIN"
run > "$T/c.txt"
changed="$( { diff <(nots "$T/a.txt") <(nots "$T/c.txt") || true; } | sed -n 's/^> \([a-z0-9_]*\)=.*/\1/p')"  # diff exits 1
[ "$changed" = "game_binary_sha256" ] || fail "(2) expected only game_binary_sha256 to change, got: '$changed'"
expect "(2)" "$T/c.txt" game_binary_sha256 "$(sha "$BIN")"

# (7) a tracked edit shows as -dirty in fork_describe
printf 'edit\n' >> "$F/README"
run > "$T/g.txt" || fail "(7) run failed"
expect "(7)" "$T/g.txt" fork_describe "base-test-dirty"
G "$F" checkout -q -- README

# (8) an uninitialised submodule (empty dir, no .git) prints absent — not the parent repo's commit
mv "$F/torch" "$T/torch-aside"; mkdir "$F/torch"
run > "$T/h.txt" || fail "(8) run failed"
expect "(8) empty dir:" "$T/h.txt" submodule_torch "absent"
expect "(8) control:" "$T/h.txt" submodule_libultraship "$(G "$F/libultraship" rev-parse HEAD)"
rmdir "$F/torch"
run > "$T/h2.txt" || fail "(8) run failed (dir gone)"
expect "(8) no dir:" "$T/h2.txt" submodule_torch "absent"
mv "$T/torch-aside" "$F/torch"

# (9) tool fields come from the right tool; a tool that prints nothing gives 'unknown', never an empty value
FB="$T/fakebin"; mkdir -p "$FB"
printf '#!/bin/sh\necho "cmake version FAKE-C"; echo second\n' > "$FB/cmake"
printf '#!/bin/sh\nexit 0\n' > "$FB/ninja"                          # exists, prints nothing
printf '#!/bin/sh\necho "clang FAKE-L"\n' > "$FB/clang"
printf '#!/bin/sh\necho /fake/sdk\n' > "$FB/xcrun"
printf '#!/bin/sh\ncase "$1" in -productVersion) echo 99.1 ;; -buildVersion) echo ZZ1 ;; esac\n' > "$FB/sw_vers"
chmod +x "$FB"/*
PATH="$FB:$PATH" run > "$T/i.txt" || fail "(9) run failed"
expect "(9)" "$T/i.txt" cmake_version "cmake version FAKE-C"
expect "(9)" "$T/i.txt" ninja_version "unknown"
expect "(9)" "$T/i.txt" clang_version "clang FAKE-L"
expect "(9)" "$T/i.txt" sdk_path      "/fake/sdk"
expect "(9)" "$T/i.txt" macos_version "99.1 (ZZ1)"
grep -q '=$' "$T/i.txt" && fail "(9) a field has an empty value: $(grep '=$' "$T/i.txt" | tr '\n' ' ')"

# (10) HOME unset (env -i): --ship-home is enough; --no-mk64 needs no ship home; neither -> exit 2 naming it
EP="/usr/bin:/bin:/opt/homebrew/bin"
set +e; env -i PATH="$EP" /bin/bash "$SUT" --fork-root "$F" --app "$APP" --ship-home "$S" --vcpkg-root "$T/vcpkg" \
  > "$T/j.txt" 2> "$T/j.err"; rc=$?; set -e
[ "$rc" -eq 0 ] || fail "(10) rc=$rc with --ship-home and no HOME: $(cat "$T/j.err")"
expect "(10)" "$T/j.txt" mk64_o2r_sha256 "$(sha "$MK")"
set +e; env -i PATH="$EP" /bin/bash "$SUT" --fork-root "$F" --app "$APP" --vcpkg-root "$T/vcpkg" --no-mk64 \
  > "$T/j2.txt" 2> "$T/j2.err"; rc=$?; set -e
[ "$rc" -eq 0 ] || fail "(10) rc=$rc with --no-mk64 and no HOME: $(cat "$T/j2.err")"
set +e; env -i PATH="$EP" /bin/bash "$SUT" --fork-root "$F" --app "$APP" --vcpkg-root "$T/vcpkg" \
  > "$T/j3.out" 2> "$T/j3.err"; rc=$?; set -e
[ "$rc" -eq 2 ] || fail "(10) rc=$rc with no HOME and no ship home, expected 2"
grep -q '^MISSING: ship home (' "$T/j3.err" || fail "(10) no 'MISSING: ship home (' on stderr: $(cat "$T/j3.err")"
[ ! -s "$T/j3.out" ] || fail "(10) partial record printed on failure"

# (11) missing vcpkg.json under --fork-root -> exit 2, 'MISSING: vcpkg.json (<path>)', no partial record
mv "$F/vcpkg.json" "$T/vcpkg.json.aside"
set +e; run > "$T/k.out" 2> "$T/k.err"; rc=$?; set -e
[ "$rc" -eq 2 ] || fail "(11) rc=$rc, expected 2"
grep -qxF "MISSING: vcpkg.json ($F/vcpkg.json)" "$T/k.err" || fail "(11) no 'MISSING: vcpkg.json ($F/vcpkg.json)' on stderr: $(cat "$T/k.err")"
[ ! -s "$T/k.out" ] || fail "(11) partial record printed on failure"
mv "$T/vcpkg.json.aside" "$F/vcpkg.json"

# (3) missing mk64.o2r -> exit 2, names it, prints NO partial record on stdout
mv "$MK" "$T/mk64.o2r.aside"
set +e; run > "$T/d.out" 2> "$T/d.err"; rc=$?; set -e
[ "$rc" -eq 2 ] || fail "(3) rc=$rc, expected 2"
grep -q 'MISSING: mk64.o2r' "$T/d.err" || fail "(3) no 'MISSING: mk64.o2r' on stderr"
[ ! -s "$T/d.out" ] || fail "(3) partial record printed on failure"

# (4) --no-mk64 (CI has no ROM) -> exit 0, explicit marker
run --no-mk64 > "$T/e.txt" || fail "(4) --no-mk64 failed"
grep -qx 'mk64_o2r_sha256=not-checked' "$T/e.txt" || fail "(4) marker missing"

# (5) unreadable input -> exit 3 naming it (permission denied)
chmod 000 "$SSE"
set +e; run --no-mk64 > /dev/null 2> "$T/f.err"; rc=$?; set -e
[ "$rc" -eq 3 ] || fail "(5) rc=$rc, expected 3"
grep -q 'UNREADABLE: sse2neon.h' "$T/f.err" || fail "(5) not named"

[ "$FAILS" -eq 0 ] || { echo "FAILED: build-identity ($FAILS checks failed)" >&2; exit 1; }
echo "PASS: build-identity (11 cases)"
