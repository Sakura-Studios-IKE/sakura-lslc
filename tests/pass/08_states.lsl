// Multiple states with transitions between them.
default
{
    state_entry()
    {
        llSetText("default", <1,1,1>, 1.0);
    }
    touch_start(integer n)
    {
        state busy;
    }
}

state busy
{
    state_entry()
    {
        llSetTimerEvent(2.0);
        llSetText("busy", <1,0,0>, 1.0);
    }
    timer()
    {
        llSetTimerEvent(0.0);
        state idle;
    }
    state_exit()
    {
        // cleanup
    }
}

state idle
{
    state_entry()
    {
        llSetText("idle", <0,1,0>, 1.0);
    }
    touch_start(integer n)
    {
        state default;
    }
}
