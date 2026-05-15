// Event: changed(integer change)
// Expected: PASS
default
{
    state_entry() { }
    changed(integer change)
    {
        if (change & CHANGED_OWNER) llResetScript();
    }
}
