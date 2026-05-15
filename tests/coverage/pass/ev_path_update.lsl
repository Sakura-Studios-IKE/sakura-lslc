// Event: path_update(integer type, list reserved)
// Expected: PASS
default
{
    state_entry() { }
    path_update(integer type, list reserved)
    {
        llOwnerSay((string)type + " n=" + (string)llGetListLength(reserved));
    }
}
