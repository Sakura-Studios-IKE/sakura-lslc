// Feature: #error inside a SKIPPED #if branch must NOT fire.
// Expected: PASS
#if 0
#error this branch is dead — must not fire
#endif

default
{
    state_entry() { llOwnerSay("alive"); }
}
