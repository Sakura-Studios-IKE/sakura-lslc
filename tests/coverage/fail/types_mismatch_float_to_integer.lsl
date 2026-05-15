// Feature: implicit assignment of float to integer is forbidden (needs explicit cast).
// Expected: FAIL
// EXPECT: cannot initialize integer with float
default
{
    state_entry()
    {
        integer x = 1.5;
        llOwnerSay((string)x);
    }
}
