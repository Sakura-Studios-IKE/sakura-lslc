// Exercises every operator supported in LSL: arithmetic, comparison,
// logical, bitwise, compound-assignment, member-access, casts.
integer       gi = 0;
float         gf = 0.0;
string        gs = "";
vector        gv;
rotation      gr;
list          gl;

default
{
    state_entry()
    {
        // Arithmetic
        integer i = 1 + 2 * 3 - 4 / 2 % 3;
        float f = 1.0 + 2.5 * 3.0;

        // Comparisons (yield integer 0/1)
        integer eq = (i == 5);
        integer ne = (i != 0);
        integer lt = (i <  10);
        integer gt = (i >  -1);

        // Logical
        integer mix = (eq && ne) || (lt && gt);

        // Bitwise
        integer m  = 0xFF & 0x0F;
        integer o  = 0xF0 | 0x0F;
        integer x  = m ^ o;
        integer y  = ~m;
        integer z  = (1 << 4) | (16 >> 2);

        // Compound assignment
        i += 1; i -= 1; i *= 2; i /= 2; i %= 7;

        // Member access
        vector v = <1.0, 2.0, 3.0>;
        v.x = 9.0;
        v.y += 0.5;
        float mag = llVecMag(v);

        rotation r = <0.0, 0.0, 0.0, 1.0>;
        r.s = 1.0;

        // String concat
        string s = "a" + "b" + "c";
        s += " more";

        // List concat
        list lst = [1, 2] + [3, 4];
        integer n = llGetListLength(lst);

        // Casts
        integer ci = (integer) "42";
        float   cf = (float)   "3.14";
        string  cs = (string)   42;
        key     ck = (key)      "00000000-0000-0000-0000-000000000000";
        vector  cv = (vector)   "<1,2,3>";
        list    cl = (list) 1 + (list) 2.0 + (list) "x";

        llOwnerSay(s + " mag=" + (string)mag + " n=" + (string)n);
    }
}
