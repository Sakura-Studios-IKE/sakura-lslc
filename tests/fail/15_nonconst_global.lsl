// Expected: globals can't call functions or reference other variables.
integer count = 0;
key     owner = llGetOwner();    // function call — not a constant
integer alias = count;           // reference to another variable

default { state_entry() {} }
