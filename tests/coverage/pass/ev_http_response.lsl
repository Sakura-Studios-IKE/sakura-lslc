// Event: http_response(key request_id, integer status, list metadata, string body)
// Expected: PASS
default
{
    state_entry() { }
    http_response(key request_id, integer status, list metadata, string body)
    {
        llOwnerSay((string)status + " " + body + " meta_n=" + (string)llGetListLength(metadata));
    }
}
