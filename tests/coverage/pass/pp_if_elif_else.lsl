// Feature: #if defined() / #elif / #else.
// Expected: PASS
#define MODE 2

#if defined(MODE) && MODE == 1
integer KIND = 1;
#elif defined(MODE) && MODE == 2
integer KIND = 2;
#elif defined(MODE)
integer KIND = 3;
#else
integer KIND = 0;
#endif

default
{
    state_entry()
    {
        llOwnerSay((string)KIND);
    }
}
