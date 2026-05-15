// Feature: if / else if / else, including nested if.
// Expected: PASS
integer classify(integer n)
{
    if (n < 0) {
        if (n < -10) return -2;
        else        return -1;
    } else if (n == 0) {
        return 0;
    } else if (n < 10) {
        return 1;
    } else {
        return 2;
    }
}

default
{
    state_entry()
    {
        integer a = classify(-100);
        integer b = classify(-1);
        integer c = classify(0);
        integer d = classify(5);
        integer e = classify(100);
        llOwnerSay((string)a + " " + (string)b + " " + (string)c + " " + (string)d + " " + (string)e);
    }
}
