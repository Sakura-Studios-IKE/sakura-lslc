// Event: at_rot_target(integer handle, rotation targetrot, rotation ourrot)
//        not_at_rot_target() — no params.
// Expected: PASS
default
{
    state_entry() { }
    at_rot_target(integer handle, rotation targetrot, rotation ourrot)
    {
        llOwnerSay((string)handle);
    }
    not_at_rot_target() { llOwnerSay("nope"); }
}
