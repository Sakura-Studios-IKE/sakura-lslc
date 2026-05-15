// Showcase a large variety of LSL event handlers in one script.
integer http_call;

default
{
    state_entry()             { llOwnerSay("hello"); }
    state_exit()              { llOwnerSay("bye");   }
    touch_start(integer n)    { llOwnerSay("touched by " + llDetectedName(0)); }
    touch(integer n)          { /* nothing */ }
    touch_end(integer n)      { /* nothing */ }
    on_rez(integer start)     { llResetScript(); }
    changed(integer c)        { if (c & CHANGED_OWNER) llResetScript(); }
    attach(key id)            { if (id == NULL_KEY) llOwnerSay("detached"); }
    timer()                   { llSetTimerEvent(0.0); }
    sensor(integer n)         { llOwnerSay("sense " + (string)n); }
    no_sensor()               { llOwnerSay("none"); }
    listen(integer ch, string name, key id, string msg) { llOwnerSay(msg); }
    money(key id, integer amount) { llOwnerSay(name_for(id) + " paid " + (string)amount); }
    object_rez(key id)        { llOwnerSay("rezzed " + (string)id); }
    run_time_permissions(integer p)   { /* nothing */ }
    transaction_result(key id, integer ok, string data) { /* nothing */ }
    link_message(integer s, integer n, string str, key id) { /* nothing */ }
    http_response(key id, integer status, list meta, string body) { /* nothing */ }
    dataserver(key q, string data) { /* nothing */ }
    moving_start() {}
    moving_end() {}
}

string name_for(key id)
{
    return llKey2Name(id);
}
