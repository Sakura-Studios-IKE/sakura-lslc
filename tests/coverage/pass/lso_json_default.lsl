// Feature: llJsonGetValue is Mono-only; under default (Mono) target it passes.
// Expected: PASS (compiled with default target)
default
{
    state_entry()
    {
        string j = "{\"a\":1}";
        string v = llJsonGetValue(j, ["a"]);
        llOwnerSay(v);
    }
}
