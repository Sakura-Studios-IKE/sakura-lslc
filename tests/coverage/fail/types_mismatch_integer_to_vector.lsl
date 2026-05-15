// Feature: integer cannot become a vector (no implicit, no explicit either).
// Expected: FAIL
// EXPECT: cannot initialize vector with integer
default
{
    state_entry()
    {
        vector v = 42;
        llOwnerSay((string)v);
    }
}
