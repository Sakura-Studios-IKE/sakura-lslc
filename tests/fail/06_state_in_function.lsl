// Expected: state change outside event handler
go_idle()
{
    state idle;
}

default { state_entry() {} }
state idle { state_entry() {} }
