// Feature: list type — mixed-element literal, casts of every typed value into list.
// Expected: PASS
list g_a = [1, 2.0, "three", NULL_KEY, <1.0, 2.0, 3.0>, <0.0, 0.0, 0.0, 1.0>];
list g_empty = [];

default
{
    state_entry()
    {
        list l = g_a + g_empty;
        // cast each kind into a list
        list a = (list) 1;
        list b = (list) 1.5;
        list c = (list) "hi";
        list d = (list) NULL_KEY;
        list e = (list) <1.0, 2.0, 3.0>;
        list f = (list) <0.0, 0.0, 0.0, 1.0>;
        // list is itself castable to string
        string s = (string) l;
        llOwnerSay("len=" + (string)llGetListLength(l) + " a+b+c+d+e+f=" + (string)(llGetListLength(a + b + c + d + e + f)) + " " + s);
    }
}
