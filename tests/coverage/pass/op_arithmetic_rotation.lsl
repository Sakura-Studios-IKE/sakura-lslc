// Feature: rotation arithmetic — rotation * rotation, rotation / rotation,
//          vector * rotation, vector / rotation.
// Expected: PASS
default
{
    state_entry()
    {
        rotation r1 = <0.0, 0.0, 0.0, 1.0>;
        rotation r2 = <0.0, 0.0, 0.707, 0.707>;
        rotation prod = r1 * r2;
        rotation quot = r1 / r2;
        rotation sum  = r1 + r2;
        rotation diff = r1 - r2;
        // vector rotated by rotation
        vector   v  = <1.0, 0.0, 0.0>;
        vector   vr = v * r2;
        vector   vd = v / r2;
        llOwnerSay("prod=" + (string)prod + " quot=" + (string)quot + " sum=" + (string)sum + " diff=" + (string)diff + " vr=" + (string)vr + " vd=" + (string)vd);
    }
}
