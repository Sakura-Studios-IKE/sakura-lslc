// Feature: implicit assignment of list to string is forbidden.
// Expected: FAIL
// EXPECT: cannot initialize string with list
default
{
    state_entry()
    {
        string s = [1, 2, 3];
        llOwnerSay(s);
    }
}
