// Feature: implicit assignment of string to integer is forbidden.
// Expected: FAIL
// EXPECT: cannot initialize integer with string
default
{
    state_entry()
    {
        integer x = "not an int";
        llOwnerSay((string)x);
    }
}
