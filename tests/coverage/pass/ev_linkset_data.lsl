// Event: linkset_data(integer action, string name, string value)
// Expected: PASS
default
{
    state_entry() { }
    linkset_data(integer action, string name, string value)
    {
        llOwnerSay((string)action + " " + name + "=" + value);
    }
}
