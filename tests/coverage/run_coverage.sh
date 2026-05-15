#!/bin/sh
# tests/coverage/run_coverage.sh
#
# Exhaustive coverage driver for sakura-lslc.
#
# Conventions inside each .lsl/.lslh file:
#   // FLAGS: <extra lslc flags>    (optional, appended once)
#   // EXPECT: <substring>          (zero or more; each must appear in stderr)
#
# Files under pass/ must compile with rc=0; if any EXPECT lines are present,
# they must all appear in the combined stdout+stderr output (warnings count).
#
# Files under fail/ must compile with rc!=0; every EXPECT line must appear
# in the combined output.
set -u

LSLC="${1:-./lslc}"
HERE="$(dirname "$0")"

# ANSI green / red bars (no -t test needed; we always emit; the user can grep)
GREEN=$(printf '\033[1;32m')
RED=$(printf '\033[1;31m')
YELLOW=$(printf '\033[1;33m')
RESET=$(printf '\033[0m')

pass=0
fail=0
total=0
unexpected_failures=""

# extract_directive FILE DIRECTIVE
# Echoes the value (everything after the colon on each matching line).
extract_directive() {
    file=$1
    name=$2
    # Match "// NAME:" with optional surrounding whitespace
    grep -E "^[[:space:]]*//[[:space:]]*${name}:" "$file" 2>/dev/null \
        | sed -E "s|^[[:space:]]*//[[:space:]]*${name}:[[:space:]]*||"
}

run_one() {
    file=$1
    expect_pass=$2     # "yes" or "no"
    total=$((total+1))

    flags=$(extract_directive "$file" FLAGS | tr '\n' ' ')
    # Run lslc; capture combined output and rc.
    out=$("$LSLC" -fno-color $flags "$file" 2>&1)
    rc=$?

    ok=1
    reason=""

    if [ "$expect_pass" = "yes" ]; then
        if [ $rc -ne 0 ]; then ok=0; reason="expected rc=0, got rc=$rc"; fi
    else
        if [ $rc -eq 0 ]; then ok=0; reason="expected rc!=0, got rc=0"; fi
    fi

    # Check EXPECT substrings (always, pass or fail).
    if [ $ok -eq 1 ]; then
        # Read each EXPECT line; ensure substring present.
        # Use a tempfile because we can't pipe-and-shellvar reliably.
        tmp=$(mktemp 2>/dev/null || mktemp -t cov)
        extract_directive "$file" EXPECT > "$tmp"
        while IFS= read -r needle; do
            [ -n "$needle" ] || continue
            case "$out" in
                *"$needle"*) ;;
                *)
                    ok=0
                    reason="missing expected substring: $needle"
                    break
                    ;;
            esac
        done < "$tmp"
        rm -f "$tmp"
    fi

    if [ $ok -eq 1 ]; then
        pass=$((pass+1))
        printf "  %sPASS%s  %s\n" "$GREEN" "$RESET" "$(basename "$file")"
    else
        fail=$((fail+1))
        unexpected_failures="$unexpected_failures\n  - $(basename "$file"): $reason"
        printf "  %sFAIL%s  %s  (%s)\n" "$RED" "$RESET" "$(basename "$file")" "$reason"
        echo "$out" | sed 's/^/        /'
    fi
}

echo "== coverage: positive tests =="
for f in "$HERE"/pass/*.lsl; do
    [ -e "$f" ] || continue
    run_one "$f" yes
done

echo "== coverage: negative tests =="
for f in "$HERE"/fail/*.lsl; do
    [ -e "$f" ] || continue
    run_one "$f" no
done

echo
if [ $fail -eq 0 ]; then
    printf "%s========================================%s\n" "$GREEN" "$RESET"
    printf "%s  ALL GREEN: %d / %d tests passed%s\n" "$GREEN" "$pass" "$total" "$RESET"
    printf "%s========================================%s\n" "$GREEN" "$RESET"
    exit 0
else
    printf "%s========================================%s\n" "$RED" "$RESET"
    printf "%s  RED: %d failed of %d (%d passed)%s\n" "$RED" "$fail" "$total" "$pass" "$RESET"
    printf "%s========================================%s\n" "$RED" "$RESET"
    printf "%bFailures:%s%b\n" "$YELLOW" "$RESET" "$unexpected_failures"
    exit 1
fi
