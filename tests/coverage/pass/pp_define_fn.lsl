// Feature: #define function-like macro with arguments.
// Expected: PASS
#define SQUARE(n) ((n) * (n))
#define ADD(a, b) ((a) + (b))
#define INC(x) ((x) + 1)

default
{
    state_entry()
    {
        integer s = SQUARE(7);
        integer total = ADD(3, 4);
        integer one_more = INC(41);
        llOwnerSay((string)s + " " + (string)total + " " + (string)one_more);
    }
}
