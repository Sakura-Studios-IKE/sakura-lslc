// Feature: 'state X;' inside a user function is forbidden.
// Expected: FAIL
// EXPECT: 'state' change is only allowed inside event handlers
go_idle() { state idle; }

default { state_entry() { go_idle(); } }
state idle { state_entry() { } }
