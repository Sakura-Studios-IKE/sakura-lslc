// Expected: cannot cast list to integer
default
{
    state_entry()
    {
        integer x = (integer)[1, 2, 3];
        vector  v = (vector)42;     // can't cast int to vector
    }
}
