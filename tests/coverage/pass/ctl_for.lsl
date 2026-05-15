// Feature: for loop with comma-separated init and step expressions.
// Expected: PASS
default
{
    state_entry()
    {
        integer total = 0;
        integer i;
        integer j;
        for (i = 0, j = 10; i < j; i++, j--) total += i + j;
        // for with empty init / empty step
        integer k = 0;
        for (; k < 3; ) k++;
        llOwnerSay("total=" + (string)total + " k=" + (string)k);
    }
}
