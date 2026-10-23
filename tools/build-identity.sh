#!/bin/bash
# build-identity.sh — diffable identity record of a macOS SpaghettiKart build (spec design consequence #6, build half).
# Usage: tools/build-identity.sh [--fork-root DIR] [--app PATH] [--ship-home DIR] [--vcpkg-root DIR] [--no-mk64]
# Exit: 0 record printed · 2 required input missing/empty/not a file · 3 unreadable · 64 usage. On non-zero, stdout is empty.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FORK_ROOT="$(dirname "$SCRIPT_DIR")"
APP=""; VCPKG_DIR="${VCPKG_ROOT:-}"; WANT_MK64=1
SHIP_DIR="${SHIP_HOME:-$HOME/Library/Application Support/SpaghettiKart}"   # src/port/Game.cpp:1050 default

usage() { echo "usage: $0 [--fork-root DIR] [--app PATH] [--ship-home DIR] [--vcpkg-root DIR] [--no-mk64]" >&2; exit 64; }
while [ "$#" -gt 0 ]; do
  case "$1" in
    --fork-root)  [ "$#" -ge 2 ] || usage; FORK_ROOT="$2"; shift 2 ;;
    --app)        [ "$#" -ge 2 ] || usage; APP="$2"; shift 2 ;;
    --ship-home)  [ "$#" -ge 2 ] || usage; SHIP_DIR="$2"; shift 2 ;;
    --vcpkg-root) [ "$#" -ge 2 ] || usage; VCPKG_DIR="$2"; shift 2 ;;
    --no-mk64)    WANT_MK64=0; shift ;;
    *)            usage ;;
  esac
done
case "$SHIP_DIR" in "~"/*) SHIP_DIR="$HOME/${SHIP_DIR#\~/}" ;; esac   # the app's own default carries a literal ~
APP="${APP:-$FORK_ROOT/build-cmake/SpaghettiKart.app}"
BUILD_DIR="$(dirname "$APP")"
VCPKG_DIR="${VCPKG_DIR:-$BUILD_DIR/.vcpkg}"   # cmake/automate-vcpkg.cmake:63 fallback on non-Windows

PROBLEMS=""; RC=0
note() { PROBLEMS="${PROBLEMS}$2: $1 ($3)"$'\n'; if [ "$4" -gt "$RC" ]; then RC="$4"; fi; return 0; }
hash_into() {  # hash_into <var> <label> <path> <required 1|0>  — sets a global (a $(...) subshell would lose PROBLEMS/RC)
  local value
  if [ ! -e "$3" ]; then value=absent; if [ "$4" = 1 ]; then note "$2" MISSING "$3" 2; fi
  elif [ -d "$3" ]; then value=not-a-file; note "$2" NOT_A_FILE "$3" 2
  elif [ ! -r "$3" ]; then value=unreadable; note "$2" UNREADABLE "$3" 3
  elif [ ! -s "$3" ]; then value=empty; note "$2" EMPTY "$3" 2
  else value="$(shasum -a 256 "$3" | awk '{print $1}')"; fi
  printf -v "$1" '%s' "$value"
}
first_line() { if command -v "$1" >/dev/null 2>&1; then { "$@" 2>/dev/null || true; } | sed -n 1p; else echo absent; fi; }
repo_head() { if [ -e "$1/.git" ]; then git -C "$1" rev-parse HEAD 2>/dev/null || echo absent; else echo absent; fi; }

hash_into game_binary  "game binary"   "$APP/Contents/MacOS/SpaghettiKart"       1
hash_into spaghetti    "spaghetti.o2r" "$APP/Contents/Resources/spaghetti.o2r"   1
hash_into config_yml   "config.yml"    "$APP/Contents/Resources/config.yml"      1
hash_into sse2neon     "sse2neon.h"    "$BUILD_DIR/_deps/sse2neon/sse2neon.h"    1   # cmake/dependencies/common.cmake:2-6
if [ "$WANT_MK64" = 1 ]; then hash_into mk64 "mk64.o2r" "$SHIP_DIR/mk64.o2r" 1; else mk64=not-checked; fi
if [ "$RC" -ne 0 ]; then printf '%s' "$PROBLEMS" >&2; exit "$RC"; fi

baseline="$(sed -n 's/.*"builtin-baseline"[[:space:]]*:[[:space:]]*"\([0-9a-f]*\)".*/\1/p' "$FORK_ROOT/vcpkg.json" 2>/dev/null | sed -n 1p)"
if command -v sw_vers >/dev/null 2>&1; then macos="$(sw_vers -productVersion) ($(sw_vers -buildVersion))"; else macos=absent; fi
cat <<EOF
fork_commit=$(repo_head "$FORK_ROOT")
fork_describe=$(git -C "$FORK_ROOT" describe --tags --always --dirty 2>/dev/null || echo absent)
submodule_libultraship=$(repo_head "$FORK_ROOT/libultraship")
submodule_torch=$(repo_head "$FORK_ROOT/torch")
vcpkg_commit=$(repo_head "$VCPKG_DIR")
vcpkg_baseline=${baseline:-absent}
cmake_version=$(first_line cmake --version)
ninja_version=$(first_line ninja --version)
clang_version=$(first_line clang --version)
macos_version=$macos
sdk_path=$(first_line xcrun --show-sdk-path)
game_binary_sha256=$game_binary
spaghetti_o2r_sha256=$spaghetti
mk64_o2r_sha256=$mk64
sse2neon_sha256=$sse2neon
config_yml_sha256=$config_yml
generated_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF
