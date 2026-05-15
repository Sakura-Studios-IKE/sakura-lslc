// Exercises every control-flow statement supported by LSL.
integer count_down(integer n)
{
    integer total = 0;
    while (n > 0) {
        total += n;
        n -= 1;
    }
    return total;
}

integer count_up(integer n)
{
    integer total = 0;
    integer i;
    for (i = 0; i < n; i++) total += i;
    return total;
}

integer fancy(integer n)
{
    integer i = 0;
    do {
        if (i == 5) jump done;
        i++;
    } while (i < n);
    @done;
    return i;
}

default
{
    state_entry()
    {
        integer a = count_down(10);
        integer b = count_up(10);
        integer c = fancy(99);

        if (a > b) llOwnerSay("a wins");
        else if (b > a) llOwnerSay("b wins");
        else llOwnerSay("tied");

        // Switch states demonstration
        if (c == 5) state idle;
    }
}

state idle
{
    state_entry()
    {
        llOwnerSay("idle");
    }
}
