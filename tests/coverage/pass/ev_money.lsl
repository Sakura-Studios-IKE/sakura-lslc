// Event: money(key id, integer amount)
// Expected: PASS
default
{
    state_entry() { }
    money(key id, integer amount) { llOwnerSay((string)id + " paid " + (string)amount); }
}
