// Feature: jump / @label, both forward and backward.
// Expected: PASS
default
{
    state_entry()
    {
        integer i = 0;
        integer hits = 0;

        // forward jump
        jump skip;
        hits = 999;
        @skip;

        // backward jump (loop with @loop and jump loop)
        @loop;
        if (i < 3) {
            ++i;
            ++hits;
            jump loop;
        }

        llOwnerSay("i=" + (string)i + " hits=" + (string)hits);
    }
}
