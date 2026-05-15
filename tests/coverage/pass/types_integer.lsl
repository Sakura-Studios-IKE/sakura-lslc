// Feature: integer type — declaration, literal forms (decimal, hex), casts in/out.
// Expected: PASS
integer g_dec = 42;
integer g_hex = 0xFF;
integer g_neg = -7;

default
{
    state_entry()
    {
        integer i = g_dec + g_hex + g_neg;
        // integer -> float
        float   f = (float) i;
        // integer -> string
        string  s = (string) i;
        // integer -> list
        list    l = (list) i;
        // round-trip from float
        integer back = (integer) f;
        // from string
        integer parsed = (integer) "42";
        llOwnerSay(s + " back=" + (string)back + " parsed=" + (string)parsed + " l_n=" + (string)llGetListLength(l));
    }
}
