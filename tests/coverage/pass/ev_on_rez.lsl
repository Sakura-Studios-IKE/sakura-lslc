// Event: on_rez(integer start_param)
// Expected: PASS
default
{
    state_entry() { }
    on_rez(integer start_param) { llOwnerSay((string)start_param); }
}
