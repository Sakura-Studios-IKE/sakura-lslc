// Feature: compound assignment operators += -= *= /= %=
// Expected: PASS
default
{
    state_entry()
    {
        integer i = 10;
        i += 5; i -= 3; i *= 2; i /= 4; i %= 3;
        float f = 10.0;
        f += 1.5; f -= 0.5; f *= 2.0; f /= 4.0;
        // vector += vector, vector *= float
        vector v = <1.0, 2.0, 3.0>;
        v += <0.5, 0.5, 0.5>;
        v -= <0.1, 0.1, 0.1>;
        v *= 2.0;
        v /= 1.5;
        // string +=
        string s = "x";
        s += "y";
        // list +=
        list l = [1, 2];
        l += [3, 4];
        l += 5;
        llOwnerSay("i=" + (string)i + " f=" + (string)f + " v=" + (string)v + " s=" + s + " l_n=" + (string)llGetListLength(l));
    }
}
