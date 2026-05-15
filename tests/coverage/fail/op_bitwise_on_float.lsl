// Feature: bitwise operators only accept integer operands.
// Expected: FAIL
// EXPECT: operator '&' is not defined for operands of type float and integer
default
{
    state_entry()
    {
        integer x = 1.5 & 0xFF;
        llOwnerSay((string)x);
    }
}
