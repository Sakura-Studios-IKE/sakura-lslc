// Feature: shift << operator only accepts integer operands.
// Expected: FAIL
// EXPECT: operator '<<' is not defined for operands of type string and integer
default
{
    state_entry()
    {
        integer x = "abc" << 1;
        llOwnerSay((string)x);
    }
}
