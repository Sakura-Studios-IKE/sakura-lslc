// Feature: vector type — declaration, literal, member access, casts.
// Expected: PASS
vector g_a = <1.0, 2.0, 3.0>;
vector g_b = <0.0, 0.0, 0.0>;

default
{
    state_entry()
    {
        vector v = g_a + <1.0, 1.0, 1.0>;
        // member access
        float  x = v.x;
        float  y = v.y;
        float  z = v.z;
        // cast vector -> string and string -> vector
        string s = (string) v;
        vector parsed = (vector) "<4, 5, 6>";
        // cast vector -> list (any single typed value)
        list l = (list) v;
        llOwnerSay(s + " x=" + (string)x + " y=" + (string)y + " z=" + (string)z + " p=" + (string)parsed + " l_n=" + (string)llGetListLength(l));
    }
}
