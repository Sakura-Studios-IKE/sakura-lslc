// Expected: wrong number of args to a built-in
default
{
    state_entry()
    {
        llOwnerSay();              // missing arg
        llOwnerSay("a", "b");      // too many args
    }
}
