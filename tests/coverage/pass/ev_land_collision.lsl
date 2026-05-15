// Events: land_collision_start, land_collision, land_collision_end — all (vector position).
// Expected: PASS
default
{
    state_entry() { }
    land_collision_start(vector position) { llOwnerSay((string)position); }
    land_collision(vector position)       { }
    land_collision_end(vector position)   { }
}
