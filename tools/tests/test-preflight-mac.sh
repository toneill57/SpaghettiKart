#!/bin/bash
# tools/tests/test-preflight-mac.sh - exit-code + output-line tests for tools/preflight-mac.sh.
# Cases force the failure they assert via env and pin the PROBLEM COUNT, so a check that prints
# but does not count is caught. Only "isolated run passes" needs network (the positive control).
# Prints "# pass N" / "# fail N"; exits 0 iff every expected case ran and passed.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUT="$HERE/../preflight-mac.sh"
EXPECTED_CASES=14
PASS=0
FAIL=0
TMP="$(mktemp -d)"
trap 'chmod -R u+rwx "$TMP" 2>/dev/null; rm -rf "$TMP"' EXIT
# Isolation: only `sh` is "required", floor 0, unresolvable host -> exactly one known failure.
ISO="PREFLIGHT_REQUIRED_TOOLS=sh PREFLIGHT_MIN_FREE_GIB=0 PREFLIGHT_NET_HOSTS=preflight-test.invalid"

run_case() {   # name want_rc regex... -- command...
  local name="$1" want_rc="$2" out rc ok=1 re
  local res=()
  shift 2
  while [ "$1" != "--" ]; do res[${#res[@]}]="$1"; shift; done
  shift
  out="$("$@" 2>&1)"; rc=$?
  [ "$rc" -eq "$want_rc" ] || ok=0
  for re in "${res[@]}"; do printf '%s\n' "$out" | grep -qE -- "$re" || ok=0; done
  if [ "$ok" -eq 1 ]; then
    echo "ok   - $name (rc=$rc)"; PASS=$((PASS + 1))
  else
    echo "FAIL - $name: want rc=$want_rc + ${res[*]}, got rc=$rc"
    printf '%s\n' "$out" | sed 's/^/     | /'
    FAIL=$((FAIL + 1))
  fi
}

[ -f "$SUT" ] || { echo "FAIL - script under test missing: $SUT"; echo "# pass 0"; echo "# fail 1"; exit 1; }

run_case "--help exits 0"                  0  '^Usage: ' -- bash "$SUT" --help
run_case "unknown flag = usage error"      64 "unknown flag '--bogus'" -- bash "$SUT" --bogus
run_case "non-numeric floor = usage error" 64 "non-negative integer, got 'abc'" -- env PREFLIGHT_MIN_FREE_GIB=abc bash "$SUT"
run_case "negative floor = usage error"    64 "non-negative integer, got '-5'" -- env PREFLIGHT_MIN_FREE_GIB=-5 bash "$SUT"
run_case "oversized floor = usage error"   64 'at most 6 digits' -- env PREFLIGHT_MIN_FREE_GIB=99999999999999999999 bash "$SUT"
run_case "minimal PATH fails loudly"       2  '^MISSING: cmake$' '^MISSING: ninja$' '^hw.ncpu: [0-9]+$' \
  -- env -i PATH=/usr/bin:/bin /bin/bash "$SUT"
run_case "offline = exactly one network problem" 2 \
  '^MISSING: network preflight-test\.invalid \(could not resolve' '^PREFLIGHT: FAIL \(1 problem\(s\)\)$' \
  -- env $ISO bash "$SUT"
run_case "floor above free space = DISK line" 2 \
  '^DISK: .* below PREFLIGHT_MIN_FREE_GIB=999999$' '^PREFLIGHT: FAIL \(2 problem\(s\)\)$' \
  -- env $ISO PREFLIGHT_MIN_FREE_GIB=999999 bash "$SUT"
run_case "missing tool is counted"         2  '^MISSING: no-such-tool-xyz$' '^PREFLIGHT: FAIL \(2 problem\(s\)\)$' \
  -- env $ISO PREFLIGHT_REQUIRED_TOOLS="sh no-such-tool-xyz" bash "$SUT"
run_case "nonexistent disk path"           2  '^MISSING: disk path /nonexistent-preflight-path$' '^PREFLIGHT: FAIL \(2 problem' \
  -- env $ISO PREFLIGHT_DISK_PATHS=/nonexistent-preflight-path bash "$SUT"
run_case "empty disk-path list"            2  "^MISSING: disk path \(PREFLIGHT_DISK_PATHS=':' names none\)$" \
  -- env $ISO PREFLIGHT_DISK_PATHS=: bash "$SUT"

mkdir -p "$TMP/locked/inner" && chmod 000 "$TMP/locked"
run_case "untraversable disk path (permission denied)" 2 "^MISSING: disk path $TMP/locked/inner$" \
  -- env $ISO PREFLIGHT_DISK_PATHS="$TMP/locked/inner" bash "$SUT"

# Concurrent access: the script writes no files, so two simultaneous runs must agree.
( env $ISO bash "$SUT" >/dev/null 2>&1; echo $? > "$TMP/rc-a" ) &
( env $ISO bash "$SUT" >/dev/null 2>&1; echo $? > "$TMP/rc-b" ) &
wait
run_case "two concurrent runs agree"       0  '^2 2$' -- sh -c "echo \$(cat '$TMP/rc-a') \$(cat '$TMP/rc-b')"

# Positive control (needs network): with nothing to fail, the script CAN exit 0.
run_case "isolated run passes"             0  '^PREFLIGHT: OK$' '^net ok: github\.com ' \
  -- env $ISO PREFLIGHT_NET_HOSTS=github.com bash "$SUT"

echo "# pass $PASS"
echo "# fail $FAIL"
[ "$((PASS + FAIL))" -eq "$EXPECTED_CASES" ] || { echo "# ran $((PASS + FAIL)) cases, expected $EXPECTED_CASES"; exit 1; }
[ "$FAIL" -eq 0 ]
