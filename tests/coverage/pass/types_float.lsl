// Feature: float type — declaration, literal forms, casts in/out.
// Expected: PASS
float g_a = 3.14;
float g_b = 1.0e2;
float g_c = .5;
float g_d = -2.0;

default
{
    state_entry()
    {
        float f = g_a + g_b + g_c + g_d;
        // implicit promote integer -> float
        float p = 1;
        // explicit -> integer (truncating)
        integer i = (integer) f;
        // -> string
        string s = (string) f;
        // -> list
        list   l = (list) f;
        // from string
        float parsed = (float) "3.14";
        llOwnerSay(s + " i=" + (string)i + " p=" + (string)p + " parsed=" + (string)parsed + " l_n=" + (string)llGetListLength(l));
    }
}
