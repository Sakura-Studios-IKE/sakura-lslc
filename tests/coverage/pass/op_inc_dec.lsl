// Feature: prefix and postfix ++ / --
// Expected: PASS
default
{
    state_entry()
    {
        integer i = 0;
        // postfix
        integer a = i++;     // a=0, i=1
        integer b = i--;     // b=1, i=0
        // prefix
        integer c = ++i;     // c=1, i=1
        integer d = --i;     // d=0, i=0
        // in loop step
        integer total = 0;
        integer j;
        for (j = 0; j < 4; ++j) total += j;
        for (j = 4; j > 0; --j) total -= 1;
        llOwnerSay("a=" + (string)a + " b=" + (string)b + " c=" + (string)c + " d=" + (string)d + " total=" + (string)total);
    }
}
