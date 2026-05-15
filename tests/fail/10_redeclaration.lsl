// Expected: redeclaration errors
integer foo = 1;
integer foo = 2;

bar() {}
bar() {}

default { state_entry() {} }
