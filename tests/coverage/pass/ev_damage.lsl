// Events: final_damage, on_damage, on_death — combat-related.
// Expected: PASS
default
{
    state_entry() { }
    final_damage(integer num_source, key victim, float damage)
    {
        llOwnerSay("final " + (string)num_source + " " + (string)damage);
    }
    on_damage(integer num_source, key id, float damage)
    {
        llOwnerSay("on " + (string)num_source + " " + (string)damage);
    }
    on_death(key id, key killer)
    {
        llOwnerSay("death " + (string)id + " by " + (string)killer);
    }
}
