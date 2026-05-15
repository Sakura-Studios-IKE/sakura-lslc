// Event: object_rez(key id)
// Expected: PASS
default
{
    state_entry() { }
    object_rez(key id) { llOwnerSay("rezzed " + (string)id); }
}
