// Feature: #warning fires (emits a warning) under -Wall (still rc=0).
// Expected: PASS
// FLAGS: -Wall
// EXPECT: #warning: deprecated path
#warning deprecated path

default { state_entry() { } }
