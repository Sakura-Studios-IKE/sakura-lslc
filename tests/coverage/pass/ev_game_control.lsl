// Event: game_control(key id, integer level)
// Expected: PASS
default
{
    state_entry() { }
    game_control(key id, integer level)
    {
        llOwnerSay((string)id + " level=" + (string)level);
    }
}
