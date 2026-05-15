// Event: timer() — no params.
// Expected: PASS
default
{
    state_entry() { llSetTimerEvent(1.0); }
    timer() { llOwnerSay("tick"); }
}
