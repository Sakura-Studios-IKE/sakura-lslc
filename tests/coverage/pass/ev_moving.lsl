// Events: moving_start() and moving_end() — no params.
// Expected: PASS
default
{
    state_entry()  { }
    moving_start() { llOwnerSay("start"); }
    moving_end()   { llOwnerSay("end"); }
}
