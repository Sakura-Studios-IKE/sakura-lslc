// Demonstrates every LSL Mono primitive type, plus the standard list type.
integer    g_int   = 42;
float      g_flt   = 3.14;
string     g_str   = "hello";
key        g_key   = NULL_KEY;
vector     g_vec   = <1.0, 2.0, 3.0>;
rotation   g_rot   = <0.0, 0.0, 0.0, 1.0>;
quaternion g_quat  = <0.0, 0.0, 0.0, 1.0>;
list       g_list  = [1, 2.0, "three", NULL_KEY];

default
{
    state_entry()
    {
        integer i = g_int + 1;
        float   f = g_flt * 2.0;
        string  s = g_str + " world";
        key     k = (key) "00000000-0000-0000-0000-000000000001";
        vector  v = g_vec + <1.0, 0.0, 0.0>;
        rotation r = g_rot * g_quat;
        list    l = g_list + ["four", 5];

        llOwnerSay(s);
    }
}
