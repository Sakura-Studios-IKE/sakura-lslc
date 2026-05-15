// Feature: #ifndef takes the "not defined" branch.
// Expected: PASS
#ifndef NOT_DEFINED
integer FLAG = 1;
#else
integer FLAG = 0;
#endif

default
{
    state_entry()
    {
        llOwnerSay((string)FLAG);
    }
}
