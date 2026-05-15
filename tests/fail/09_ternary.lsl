// Expected: ?: is not allowed in LSL
default
{
    state_entry()
    {
        integer x = 1 ? 2 : 3;
    }
}
