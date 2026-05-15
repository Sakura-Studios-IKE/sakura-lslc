// Events: touch_start, touch, touch_end — all (integer total_number).
// Expected: PASS
default
{
    state_entry() { }
    touch_start(integer total_number) { llOwnerSay((string)total_number); }
    touch(integer total_number)       { llOwnerSay("hold " + (string)total_number); }
    touch_end(integer total_number)   { llOwnerSay("end " + (string)total_number); }
}
