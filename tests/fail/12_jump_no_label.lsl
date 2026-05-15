// Expected: jump target without matching label
default
{
    state_entry()
    {
        jump missing;
    }
}
