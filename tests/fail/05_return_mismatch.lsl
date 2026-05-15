// Expected: return type mismatch and missing return
integer needs_int()
{
    return "not an integer";
}

vector needs_vec()
{
    return;     // missing value
}

void_proc()
{
    return 1;   // returning a value from a void fn
}

default { state_entry() {} }
