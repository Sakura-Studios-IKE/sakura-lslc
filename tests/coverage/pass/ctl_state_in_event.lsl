// Feature: 'state X;' is legal inside an event handler.
// Expected: PASS
default
{
    state_entry()
    {
        state idle;
    }
}

state idle
{
    state_entry()
    {
        llOwnerSay("idle");
        state default;
    }
}
