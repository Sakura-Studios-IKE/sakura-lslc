// Feature: -Wall surfaces an "unused global variable" warning, but doesn't fail.
// Expected: PASS
// FLAGS: -Wall
// EXPECT: unused global variable 'unused_var'
integer unused_var = 7;

default
{
    state_entry() { llOwnerSay("hi"); }
}
