// Event: attach(key id)
// Expected: PASS
default
{
    state_entry() { }
    attach(key id) { if (id == NULL_KEY) llOwnerSay("detached"); }
}
