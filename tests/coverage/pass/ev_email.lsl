// Event: email(string time, string address, string subject, string body, integer num_left)
// Expected: PASS
default
{
    state_entry() { }
    email(string time, string address, string subject, string body, integer num_left)
    {
        llOwnerSay(time + " " + address + " " + subject + " " + body + " " + (string)num_left);
    }
}
