// Expected: undeclared identifier error
default
{
    state_entry()
    {
        llOwnerSay(undeclared_var);
    }
}
