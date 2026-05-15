// Pulls in helper definitions via #include.
#include "13_preproc_include_lib.lslh"

default
{
    state_entry()
    {
        llOwnerSay((string)add(mul(3, 4), 5));
    }
}
