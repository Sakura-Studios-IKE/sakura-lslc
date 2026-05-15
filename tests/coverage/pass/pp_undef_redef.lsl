// Feature: #undef followed by #define re-defines the same name.
// Expected: PASS
#define X 1
#undef X
#define X 2

default
{
    state_entry()
    {
        integer x = X;     // -> 2
        llOwnerSay((string)x);
    }
}
