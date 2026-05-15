// Feature: user functions — void, each return type, multiple params, recursion.
// Expected: PASS
nothing() { llOwnerSay("ok"); }
integer  ret_int(integer a, integer b) { return a + b; }
float    ret_float(float a, float b)   { return a * b; }
string   ret_string(string a, string b){ return a + b; }
key      ret_key()                     { return NULL_KEY; }
vector   ret_vec()                     { return <1.0, 2.0, 3.0>; }
rotation ret_rot()                     { return <0.0, 0.0, 0.0, 1.0>; }
list     ret_list(integer a, float b, string c) { return [a, b, c]; }

integer fact(integer n)
{
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

default
{
    state_entry()
    {
        nothing();
        integer a = ret_int(2, 3);
        float   b = ret_float(2.0, 3.5);
        string  c = ret_string("x", "y");
        key     d = ret_key();
        vector  e = ret_vec();
        rotation f = ret_rot();
        list    g = ret_list(1, 2.0, "x");
        integer fa = fact(6);
        llOwnerSay("a=" + (string)a + " b=" + (string)b + " c=" + c + " e=" + (string)e + " fa=" + (string)fa + " g_n=" + (string)llGetListLength(g) + " d=" + (string)d + " f=" + (string)f);
    }
}
