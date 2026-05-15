// Event: state_exit() — no params.
// Expected: PASS
default
{
    state_entry() { state idle; }
    state_exit()  { llOwnerSay("exiting"); }
}
state idle { state_entry() { } }
