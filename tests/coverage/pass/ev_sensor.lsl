// Events: sensor(integer num_detected) and no_sensor() — no-params.
// Expected: PASS
default
{
    state_entry() { }
    sensor(integer num_detected) { llOwnerSay((string)num_detected); }
    no_sensor() { llOwnerSay("none"); }
}
