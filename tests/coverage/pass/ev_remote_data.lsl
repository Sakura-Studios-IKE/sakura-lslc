// Event: remote_data(integer event_type, key channel, key message_id)
// Expected: PASS
default
{
    state_entry() { }
    remote_data(integer event_type, key channel, key message_id)
    {
        llOwnerSay((string)event_type + " " + (string)channel + " " + (string)message_id);
    }
}
