// Event: listen(integer channel, string name, key id, string message).
// Expected: PASS
default
{
    state_entry() { }
    listen(integer channel, string name, key id, string message)
    {
        llOwnerSay((string)channel + " " + name + " " + (string)id + ": " + message);
    }
}
