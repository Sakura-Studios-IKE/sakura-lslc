// Event: run_time_permissions(integer perm)
// Expected: PASS
default
{
    state_entry() { }
    run_time_permissions(integer perm) { llOwnerSay((string)perm); }
}
