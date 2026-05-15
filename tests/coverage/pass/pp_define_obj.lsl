// Feature: #define object-like macro.
// Expected: PASS
#define ANSWER 42
#define GREETING "hi"

default
{
    state_entry()
    {
        integer x = ANSWER;
        string  s = GREETING;
        llOwnerSay(s + " " + (string)x);
    }
}
