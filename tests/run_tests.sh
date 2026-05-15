#!/bin/sh
# run_tests.sh - exercise lslc against the in-tree pass/fail corpora.
set -u
LSLC="${1:-./lslc}"
HERE="$(dirname "$0")"

pass=0
fail=0
unexpected=0

echo "== positive tests =="
for f in "$HERE"/pass/*.lsl; do
    [ -e "$f" ] || continue
    out=$("$LSLC" -fno-color "$f" 2>&1)
    rc=$?
    if [ $rc -eq 0 ]; then
        pass=$((pass+1))
        printf "  PASS  %s\n" "$(basename "$f")"
    else
        fail=$((fail+1))
        unexpected=$((unexpected+1))
        printf "  FAIL  %s (expected success)\n" "$(basename "$f")"
        echo "$out" | sed 's/^/      /'
    fi
done

echo "== negative tests =="
for f in "$HERE"/fail/*.lsl; do
    [ -e "$f" ] || continue
    out=$("$LSLC" -fno-color "$f" 2>&1)
    rc=$?
    if [ $rc -ne 0 ]; then
        pass=$((pass+1))
        printf "  PASS  %s (rejected as expected)\n" "$(basename "$f")"
    else
        fail=$((fail+1))
        unexpected=$((unexpected+1))
        printf "  FAIL  %s (expected failure)\n" "$(basename "$f")"
    fi
done

echo
echo "Result: $pass passing / $((pass+fail)) tests"
test $unexpected -eq 0
