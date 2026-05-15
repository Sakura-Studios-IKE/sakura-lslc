// Expected: event signature mismatch
default
{
    // touch_start should take (integer total_number) but we wrote 'string'
    touch_start(string n)
    {
    }

    // Wrong number of args
    on_rez(integer a, integer b)
    {
    }

    // Unknown event name
    state_entrey()
    {
    }
}
