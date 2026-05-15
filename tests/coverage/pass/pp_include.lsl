// Feature: #include with a relative path. Helper lives in ../include/.
// Expected: PASS
#include "../include/helper.lslh"

default
{
    state_entry()
    {
        integer x = my_add(my_mul(3, 4), 5);
        llOwnerSay((string)x);
    }
}
