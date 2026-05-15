// Feature: list + anything (and anything + list) yields a list.
// Expected: PASS
default
{
    state_entry()
    {
        list l = [1, 2, 3];
        list a = l + 4;
        list b = l + 1.5;
        list c = l + "hi";
        list d = l + NULL_KEY;
        list e = l + <1.0, 0.0, 0.0>;
        list f = l + <0.0, 0.0, 0.0, 1.0>;
        list g = l + l;
        // prepend
        list h = 0 + l;
        llOwnerSay((string)llGetListLength(a + b + c + d + e + f + g + h));
    }
}
