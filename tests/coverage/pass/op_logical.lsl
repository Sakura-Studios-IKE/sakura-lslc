// Feature: logical operators && || !
// Expected: PASS
default
{
    state_entry()
    {
        integer a = 1;
        integer b = 0;
        integer x = a && b;
        integer y = a || b;
        integer z = !a;
        integer w = !!b;
        integer combined = (a && !b) || (b && !a);
        llOwnerSay("x=" + (string)x + " y=" + (string)y + " z=" + (string)z + " w=" + (string)w + " c=" + (string)combined);
    }
}
