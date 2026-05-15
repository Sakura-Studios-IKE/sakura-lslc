// User-defined functions: with and without return types, multiple params,
// recursion, forward references between definitions.
integer fib(integer n)
{
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

vector unit_x()
{
    return <1.0, 0.0, 0.0>;
}

string greet(string who, integer times)
{
    string out = "";
    integer i;
    for (i = 0; i < times; i++) out += "hello " + who + "\n";
    return out;
}

// procedure with no return type
say_loud(string msg)
{
    llOwnerSay(llToUpper(msg));
}

default
{
    state_entry()
    {
        integer x = fib(10);
        vector  v = unit_x();
        string  g = greet("world", 2);
        say_loud(g);
        llOwnerSay("fib(10) = " + (string)x + ", v = " + (string)v);
    }
}
