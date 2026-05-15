// Feature: vector cannot be assigned to rotation (distinct types).
// Expected: FAIL
// EXPECT: cannot initialize rotation with vector
default
{
    state_entry()
    {
        rotation r = <1.0, 2.0, 3.0>;
        llOwnerSay((string)r);
    }
}
