// Event: at_target(integer tnum, vector targetpos, vector ourpos)
//        not_at_target() — no params.
// Expected: PASS
default
{
    state_entry() { }
    at_target(integer tnum, vector targetpos, vector ourpos)
    {
        llOwnerSay((string)tnum);
    }
    not_at_target() { llOwnerSay("missed"); }
}
