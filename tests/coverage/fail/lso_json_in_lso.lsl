// Feature: llJsonGetValue under --lso is rejected (Mono-only built-in).
// Expected: FAIL (must be compiled with --lso)
// FLAGS: --lso
// EXPECT: 'llJsonGetValue' is a Mono-only built-in and is not available under LSO
default
{
    state_entry()
    {
        string j = "{\"a\":1}";
        string v = llJsonGetValue(j, ["a"]);
        llOwnerSay(v);
    }
}
