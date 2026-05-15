// Feature: quaternion is an alias for rotation — assignable both ways.
// Expected: PASS
quaternion g_q = <0.0, 0.0, 0.0, 1.0>;
rotation   g_r = <0.0, 0.0, 0.0, 1.0>;

default
{
    state_entry()
    {
        // assign each into the other without explicit cast
        quaternion q = g_r;
        rotation   r = g_q;
        // member s exists on both
        float qs = q.s;
        float rs = r.s;
        llOwnerSay("q.s=" + (string)qs + " r.s=" + (string)rs);
    }
}
