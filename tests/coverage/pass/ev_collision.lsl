// Events: collision_start, collision, collision_end — all (integer num_detected).
// Expected: PASS
default
{
    state_entry() { }
    collision_start(integer num_detected) { llOwnerSay((string)num_detected); }
    collision(integer num_detected)       { }
    collision_end(integer num_detected)   { }
}
