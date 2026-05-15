// Expected: vector has no member 'w'; rotation has no member 'q'
default
{
    state_entry()
    {
        vector v = <0,0,0>;
        float a = v.w;        // vectors have only x y z
        rotation r = <0,0,0,1>;
        float b = r.q;        // rotations only have x y z s
    }
}
