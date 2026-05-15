// Exercise labels and jumps including forward references.
integer search(list haystack, string needle)
{
    integer i = 0;
    integer n = llGetListLength(haystack);
    @loop;
    if (i >= n) jump miss;
    if (llList2String(haystack, i) == needle) jump hit;
    i++;
    jump loop;
    @hit;
    return i;
    @miss;
    return -1;
}

default
{
    state_entry()
    {
        integer r = search(["a", "b", "c"], "b");
        llOwnerSay("r = " + (string)r);
    }
}
