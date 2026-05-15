// Feature: member access — v.x .y .z on vectors; r.x .y .z .s on rotations.
//          LHS assignment to a member is also tested.
// Expected: PASS
default
{
    state_entry()
    {
        vector v = <1.0, 2.0, 3.0>;
        float vx = v.x;
        float vy = v.y;
        float vz = v.z;
        v.x = 9.0;
        v.y += 0.5;
        v.z *= 2.0;

        rotation r = <0.0, 0.0, 0.0, 1.0>;
        float rx = r.x;
        float ry = r.y;
        float rz = r.z;
        float rs = r.s;
        r.s = 1.0;
        r.x += 0.1;
        llOwnerSay("v=" + (string)v + " vx=" + (string)vx + " vy=" + (string)vy + " vz=" + (string)vz + " r=" + (string)r + " rs=" + (string)rs);
    }
}
