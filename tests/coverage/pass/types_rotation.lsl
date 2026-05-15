// Feature: rotation type — declaration, literal, member access (x y z s), casts.
// Expected: PASS
rotation g_a = <0.0, 0.0, 0.0, 1.0>;

default
{
    state_entry()
    {
        rotation r = g_a;
        // four members
        float x = r.x;
        float y = r.y;
        float z = r.z;
        float s = r.s;
        // cast to string and back
        string  str    = (string) r;
        rotation parsed = (rotation) "<0, 0, 0, 1>";
        // cast to list
        list l = (list) r;
        llOwnerSay(str + " x=" + (string)x + " y=" + (string)y + " z=" + (string)z + " s=" + (string)s + " p=" + (string)parsed + " l_n=" + (string)llGetListLength(l));
    }
}
