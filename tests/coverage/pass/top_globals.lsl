// Feature: global variable declarations for every primitive + list.
// Expected: PASS
integer    g_int  = 7;
float      g_flt  = 1.5;
string     g_str  = "hi";
key        g_key  = NULL_KEY;
vector     g_vec  = <1.0, 2.0, 3.0>;
rotation   g_rot  = <0.0, 0.0, 0.0, 1.0>;
quaternion g_quat = <0.0, 0.0, 0.0, 1.0>;
list       g_lst  = [1, 2.0, "three"];
// uninitialized global (gets default value)
integer    g_zero;
vector     g_zv;

default
{
    state_entry()
    {
        llOwnerSay("globals ok: " + (string)g_int + " " + (string)g_flt + " " + g_str + " z=" + (string)g_zero + " zv=" + (string)g_zv);
    }
}
