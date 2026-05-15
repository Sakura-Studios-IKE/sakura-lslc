// Feature: return statement — with and without value, in functions and events.
// Expected: PASS
integer with_value(integer x) { return x + 1; }
no_value() { return; }
just_an_event_helper() { /* no return needed */ }

default
{
    state_entry()
    {
        integer a = with_value(41);
        no_value();
        just_an_event_helper();
        // bare 'return' inside an event handler is also legal
        if (a > 0) return;
        llOwnerSay("unreachable");
    }
    touch_start(integer n)
    {
        return;  // valid: early exit from event
    }
}
