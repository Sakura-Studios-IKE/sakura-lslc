// Global initializers are constant expressions; the compiler folds them.
integer  N    = 60 * 60 * 24;            // -> 86400
float    AREA = 3.14 * 5.0 * 5.0;        // -> ~78.5
string   GREET = "hello, " + "world";    // -> "hello, world"
integer  MASK = (1 << 4) | (1 << 2);     // -> 20
vector   AXIS = <1.0, 0.0, 0.0>;
vector   DOUBLE = <1.0, 2.0, 3.0> * 2.0; // Mono does not fold this — keep as-is

integer  USES_CONST = PI > 3.0;          // built-in PI is a constant

default
{
    state_entry()
    {
        llOwnerSay((string)N + " " + (string)AREA + " " + GREET);
    }
}
