// Events: experience_permissions(key agent_id) and experience_permissions_denied(key, integer)
// Expected: PASS
default
{
    state_entry() { }
    experience_permissions(key agent_id) { llOwnerSay((string)agent_id); }
    experience_permissions_denied(key agent_id, integer reason)
    {
        llOwnerSay((string)agent_id + ":" + (string)reason);
    }
}
