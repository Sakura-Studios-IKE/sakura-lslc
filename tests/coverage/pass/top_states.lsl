// Feature: default state plus multiple named states.
// Expected: PASS
default
{
    state_entry()    { state running; }
    state_exit()     { /* leaving default */ }
}

state running
{
    state_entry()    { llOwnerSay("running"); }
    state_exit()     { llOwnerSay("leaving running"); }
    touch_start(integer n) { state paused; }
}

state paused
{
    state_entry()    { llOwnerSay("paused"); }
    touch_start(integer n) { state running; }
}
