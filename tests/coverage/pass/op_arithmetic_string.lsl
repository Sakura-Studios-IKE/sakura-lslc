// Feature: string + string is concatenation.
// Expected: PASS
default
{
    state_entry()
    {
        string a = "hello, " + "world";
        string b = a + "!";
        // accumulate via +=
        string c = "";
        c += "x"; c += "y"; c += "z";
        llOwnerSay(a + " | " + b + " | " + c);
    }
}
