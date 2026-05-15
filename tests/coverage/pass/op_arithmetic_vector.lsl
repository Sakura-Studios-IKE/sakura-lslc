// Feature: vector arithmetic — vector + vector, vector - vector, vector * float,
//          float * vector, vector / float, vector * vector (dot), vector % vector (cross).
// Expected: PASS
default
{
    state_entry()
    {
        vector a = <1.0, 2.0, 3.0>;
        vector b = <4.0, 5.0, 6.0>;
        vector sum  = a + b;
        vector diff = a - b;
        vector neg  = -a;
        // scalar mul both sides
        vector vs   = a * 2.0;
        vector sv   = 2.0 * a;
        vector vd   = a / 2.0;
        // dot product is float
        float  dot  = a * b;
        // cross product is vector
        vector cross = a % b;
        llOwnerSay("sum=" + (string)sum + " diff=" + (string)diff + " neg=" + (string)neg + " vs=" + (string)vs + " sv=" + (string)sv + " vd=" + (string)vd + " dot=" + (string)dot + " cross=" + (string)cross);
    }
}
