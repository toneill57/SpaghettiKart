#!/bin/bash
# tools/preflight-mac.sh - macOS pre-flight for the MK64 Online fork (Fase 0.2).
# Prints the host/toolchain facts the first configure depends on, then exits:
#   0  every required tool resolves, every disk path has >= PREFLIGHT_MIN_FREE_GIB free,
#      and every host answers HTTPS
#   2  one or more "MISSING:" / "DISK:" lines (ALL problems are listed, not just the first)
#   64 usage error (unknown flag, bad PREFLIGHT_MIN_FREE_GIB)
# Env: PREFLIGHT_MIN_FREE_GIB (default 30) · PREFLIGHT_DISK_PATHS (':'-separated, default =
#      the fork's parent dir) · PREFLIGHT_NET_HOSTS (space-separated, default github.com
#      raw.githubusercontent.com) · PREFLIGHT_REQUIRED_TOOLS (test isolation only).
# Writes no files. bash 3.2 compatible (macOS /bin/bash).
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FORK_ROOT="$(dirname "$SCRIPT_DIR")"

# Build tools + the Apple tools cmake/macos/apple_bundle.cmake shells out to (:98 sips,
# :100 iconutil, :112 plutil, :150 install_name_tool, :163 codesign), ditto (CI zip, main.yml),
# shasum (ROM/identity hashes), otool (BundleUtilities inside cmake/macos/fixup_bundle.cmake:7).
DEFAULT_TOOLS="cmake ninja git clang xcodebuild xcode-select sips iconutil plutil codesign install_name_tool ditto shasum otool curl"
REQUIRED_TOOLS="${PREFLIGHT_REQUIRED_TOOLS:-$DEFAULT_TOOLS}"   # override = test isolation only
MIN_FREE_GIB="${PREFLIGHT_MIN_FREE_GIB:-30}"
MAX_FLOOR_DIGITS=6
DISK_PATHS="${PREFLIGHT_DISK_PATHS:-$(dirname "$FORK_ROOT")}"
NET_HOSTS="${PREFLIGHT_NET_HOSTS:-github.com raw.githubusercontent.com}"
NET_TIMEOUT_S=10
SYSCTL=/usr/sbin/sysctl   # not on PATH=/usr/bin:/bin
EXIT_FAIL=2
EXIT_USAGE=64
FAILURES=0

usage() {
  echo "Usage: tools/preflight-mac.sh [--help]"
  echo "  env PREFLIGHT_MIN_FREE_GIB=<int>  PREFLIGHT_DISK_PATHS=<p1:p2>  PREFLIGHT_NET_HOSTS='<h1 h2>'"
}

fail() { echo "$1"; FAILURES=$((FAILURES + 1)); }

for arg in "$@"; do
  case "$arg" in
    -h|--help) usage; exit 0 ;;
    *) echo "ERROR: unknown flag '$arg'" >&2; usage >&2; exit "$EXIT_USAGE" ;;
  esac
done

case "$MIN_FREE_GIB" in
  ''|*[!0-9]*)
    echo "ERROR: PREFLIGHT_MIN_FREE_GIB must be a non-negative integer, got '$MIN_FREE_GIB'" >&2
    exit "$EXIT_USAGE" ;;
esac
if [ "${#MIN_FREE_GIB}" -gt "$MAX_FLOOR_DIGITS" ]; then
  echo "ERROR: PREFLIGHT_MIN_FREE_GIB must have at most $MAX_FLOOR_DIGITS digits, got '$MIN_FREE_GIB'" >&2
  exit "$EXIT_USAGE"
fi

first_line() { "$@" 2>&1 | sed -n 1p; }   # sed reads to EOF: no SIGPIPE under pipefail

echo "== preflight-mac $(date -u +%Y-%m-%dT%H:%M:%SZ) fork_root=$FORK_ROOT"
echo "PATH: $PATH"
echo "== host"
echo "uname -m: $(uname -m)"
echo "sw_vers: $(sw_vers -productVersion 2>&1) ($(sw_vers -buildVersion 2>&1))"
echo "hw.ncpu: $("$SYSCTL" -n hw.ncpu)"
MEM_BYTES="$("$SYSCTL" -n hw.memsize)"
echo "hw.memsize: $MEM_BYTES ($((MEM_BYTES / 1073741824)) GiB)"

echo "== tools"
for tool in $REQUIRED_TOOLS; do
  tool_path="$(command -v "$tool" 2>/dev/null || true)"
  if [ -z "$tool_path" ]; then fail "MISSING: $tool"; else echo "found: $tool -> $tool_path"; fi
done

echo "== versions"
for spec in "cmake --version" "ninja --version" "git --version" "clang --version"; do
  set -- $spec
  if command -v "$1" >/dev/null 2>&1; then echo "$1: $(first_line "$@")"; fi
done
if command -v xcodebuild >/dev/null 2>&1; then
  if XCB="$(xcodebuild -version 2>&1)"; then
    echo "xcodebuild: $(printf '%s' "$XCB" | tr '\n' ' ')"
  else
    fail "MISSING: Xcode.app (xcodebuild -version failed; xcode-select -p = $(xcode-select -p 2>&1))"
  fi
fi
echo "xcode-select -p: $(xcode-select -p 2>&1)"
echo "VCPKG_ROOT: ${VCPKG_ROOT:-<unset>}"
echo "VCPKG_DEFAULT_BINARY_CACHE: ${VCPKG_DEFAULT_BINARY_CACHE:-<unset>}"

check_disk() {
  local p="$1" line avail
  if [ ! -d "$p" ]; then fail "MISSING: disk path $p"; return; fi
  line="$(df -g "$p" 2>&1 | tail -1)"
  echo "df -g $p: $line"
  avail="$(printf '%s\n' "$line" | awk '{print $4}')"
  case "$avail" in ''|*[!0-9]*) fail "DISK: cannot read free GiB for $p from: $line"; return ;; esac
  if [ "$avail" -lt "$MIN_FREE_GIB" ]; then
    fail "DISK: $p has $avail GiB free, below PREFLIGHT_MIN_FREE_GIB=$MIN_FREE_GIB"
  else
    echo "disk ok: $p $avail GiB free (floor $MIN_FREE_GIB)"
  fi
}

echo "== disk (floor $MIN_FREE_GIB GiB)"
CHECKED=0
OLD_IFS="$IFS"; IFS=':'; set -f
for p in $DISK_PATHS; do
  IFS="$OLD_IFS"
  [ -n "$p" ] || continue
  CHECKED=$((CHECKED + 1))
  check_disk "$p"
done
IFS="$OLD_IFS"; set +f
[ "$CHECKED" -gt 0 ] || fail "MISSING: disk path (PREFLIGHT_DISK_PATHS='$DISK_PATHS' names none)"

check_host() {
  local host="$1" dns code rc
  if nslookup "$host" >/dev/null 2>&1; then dns=ok; else dns=fail; fi   # informational only
  code="$(curl -sS -o /dev/null -I --max-time "$NET_TIMEOUT_S" -w '%{http_code}' "https://$host/" 2>/dev/null)"
  rc=$?
  case "$rc" in
    0)  echo "net ok: $host dns=$dns https=$code" ;;
    6)  fail "MISSING: network $host (could not resolve host, curl rc=6, nslookup=$dns)" ;;
    7)  fail "MISSING: network $host (connection refused, curl rc=7)" ;;
    28) fail "MISSING: network $host (timeout after ${NET_TIMEOUT_S}s, curl rc=28)" ;;
    *)  fail "MISSING: network $host (curl rc=$rc, nslookup=$dns)" ;;
  esac
}

echo "== network"
if command -v curl >/dev/null 2>&1; then
  for host in $NET_HOSTS; do check_host "$host"; done
else
  echo "network: not checked (curl missing, already reported)"
fi

echo "== result"
if [ "$FAILURES" -gt 0 ]; then
  echo "PREFLIGHT: FAIL ($FAILURES problem(s))"
  exit "$EXIT_FAIL"
fi
echo "PREFLIGHT: OK"
exit 0
