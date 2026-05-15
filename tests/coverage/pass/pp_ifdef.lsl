// Feature: #ifdef / #else / #endif — both branches taken across two defines.
// Expected: PASS
#define HAS_LOGGING

#ifdef HAS_LOGGING
integer LOGGING = 1;
#else
integer LOGGING = 0;
#endif

#ifdef MISSING
integer MIA = 1;
#else
integer MIA = 0;
#endif

default
{
    state_entry()
    {
        llOwnerSay((string)LOGGING + " " + (string)MIA);
    }
}
