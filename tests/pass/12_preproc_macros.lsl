// Exercises object-like and function-like macros, ifdef, undef.
#define VERSION "1.0.0"
#define MAX_RETRIES 5
#define SQUARE(n) ((n) * (n))

#define USE_LOGGING

string GREETING = "v" + VERSION;

#ifdef USE_LOGGING
log_msg(string m) { llOwnerSay("[" + VERSION + "] " + m); }
#else
log_msg(string m) { }
#endif

default
{
    state_entry()
    {
        integer x = SQUARE(7);          // -> 49
        integer retries = MAX_RETRIES;  // -> 5
        log_msg("hi");
        llOwnerSay((string)x + " " + (string)retries);
    }
}

#undef USE_LOGGING
#ifndef USE_LOGGING
// after undef, this branch is taken — but only the comment is emitted
#endif
