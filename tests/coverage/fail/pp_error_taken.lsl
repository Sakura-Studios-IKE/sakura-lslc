// Feature: #error inside a TAKEN branch fires.
// Expected: FAIL
// EXPECT: #error: this branch is taken
#if 1
#error this branch is taken — must fire
#endif

default { state_entry() { } }
