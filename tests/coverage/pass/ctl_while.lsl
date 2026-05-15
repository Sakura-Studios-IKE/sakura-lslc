// Feature: while loop.
// Expected: PASS
default
{
    state_entry()
    {
        integer n = 10;
        integer sum = 0;
        while (n > 0) {
            sum += n;
            n -= 1;
        }
        // empty body still legal
        integer k = 0;
        while (k < 3) ++k;
        llOwnerSay("sum=" + (string)sum + " k=" + (string)k);
    }
}
