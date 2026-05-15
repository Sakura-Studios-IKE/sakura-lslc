// Feature: string type — declaration, escape sequences, casts in/out.
// Expected: PASS
string g_a = "hello";
string g_b = "with \"quotes\" and \n newline";
string g_c = "";

default
{
    state_entry()
    {
        string s = g_a + " " + g_b + g_c;
        // string is castable to/from every primitive
        integer i  = (integer) "123";
        float   f  = (float)   "1.5";
        key     k  = (key)     "00000000-0000-0000-0000-000000000001";
        vector  v  = (vector)  "<1, 2, 3>";
        rotation r = (rotation)"<0, 0, 0, 1>";
        // string can be appended into lists
        list    l  = (list) s;
        llOwnerSay(s + " i=" + (string)i + " f=" + (string)f + " k=" + (string)k + " v=" + (string)v + " r=" + (string)r + " l_n=" + (string)llGetListLength(l));
    }
}
