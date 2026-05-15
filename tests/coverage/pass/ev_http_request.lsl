// Event: http_request(key id, string method, string body) (3-arg variant)
// Expected: PASS
default
{
    state_entry() { }
    http_request(key id, string method, string body)
    {
        llOwnerSay((string)id + " " + method + " " + body);
    }
}
