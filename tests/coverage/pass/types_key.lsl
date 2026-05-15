// Feature: key type — declaration, NULL_KEY constant, implicit and explicit casts.
// Expected: PASS
key g_a = NULL_KEY;
key g_b = "00000000-0000-0000-0000-000000000001";

default
{
    state_entry()
    {
        // implicit: string -> key, key -> string in assignment context
        key    k = "00000000-0000-0000-0000-000000000002";
        string s = g_b;
        // explicit cast both ways
        key    ek = (key) "00000000-0000-0000-0000-000000000003";
        string es = (string) g_a;
        // -> list
        list l = (list) k;
        llOwnerSay(s + " ek=" + (string)ek + " es=" + es + " l_n=" + (string)llGetListLength(l));
    }
}
