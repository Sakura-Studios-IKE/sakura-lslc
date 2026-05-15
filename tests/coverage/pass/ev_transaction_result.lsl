// Event: transaction_result(key id, integer success, string data)
// Expected: PASS
default
{
    state_entry() { }
    transaction_result(key id, integer success, string data)
    {
        llOwnerSay((string)id + ":" + (string)success + ":" + data);
    }
}
