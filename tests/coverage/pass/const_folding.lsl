// Feature: constant-folded global initialisers — literal arithmetic, string concat,
//          vector member access on a vector literal, casts.
// Expected: PASS
integer  SECONDS = 60 * 60 * 24;          // 86400
float    AREA    = 3.14 * 5.0 * 5.0;
string   GREET   = "hello, " + "world";
integer  BITS    = (1 << 4) | (1 << 2);
// member access on a literal vector (must be constant-foldable for global init)
float    AXIS_X  = <1.0, 2.0, 3.0>.x;
float    AXIS_Y  = <1.0, 2.0, 3.0>.y;
// cast of a literal
integer  FROM_F  = (integer) 3.9;
string   FROM_I  = (string)  42;
// built-in constant in an initialiser
integer  HAS_PI  = PI > 3.0;

default
{
    state_entry()
    {
        llOwnerSay((string)SECONDS + " " + GREET + " bits=" + (string)BITS + " ax=" + (string)AXIS_X + " ay=" + (string)AXIS_Y + " fi=" + (string)FROM_F + " fs=" + FROM_I + " hp=" + (string)HAS_PI);
    }
}
