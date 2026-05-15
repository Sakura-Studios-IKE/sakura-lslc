// Feature: do-while loop. Body executes at least once.
// Expected: PASS
default
{
    state_entry()
    {
        integer i = 0;
        integer s = 0;
        do {
            s += i;
            i += 1;
        } while (i < 5);
        // single-iteration body
        integer j = 100;
        do { j -= 50; } while (j > 80);
        llOwnerSay("s=" + (string)s + " j=" + (string)j);
    }
}
