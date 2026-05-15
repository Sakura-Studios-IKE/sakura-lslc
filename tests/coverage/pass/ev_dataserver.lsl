// Event: dataserver(key queryid, string data)
// Expected: PASS
default
{
    state_entry() { }
    dataserver(key queryid, string data) { llOwnerSay((string)queryid + ":" + data); }
}
