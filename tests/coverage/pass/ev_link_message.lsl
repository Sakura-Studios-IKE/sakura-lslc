// Event: link_message(integer sender_num, integer num, string str, key id)
// Expected: PASS
default
{
    state_entry() { }
    link_message(integer sender_num, integer num, string str, key id)
    {
        llOwnerSay((string)sender_num + ":" + (string)num + " " + str + " " + (string)id);
    }
}
