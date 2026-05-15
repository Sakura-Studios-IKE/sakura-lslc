// Feature: == != < > <= >= on compatible types. All yield integer 0/1.
// Expected: PASS
default
{
    state_entry()
    {
        // integer vs integer
        integer a = (1 == 1) + (1 != 2) + (1 < 2) + (2 > 1) + (1 <= 1) + (1 >= 1);
        // float vs float
        integer b = (1.0 == 1.0) + (1.0 != 2.0) + (1.0 < 2.0) + (2.0 > 1.0) + (1.0 <= 1.0) + (1.0 >= 1.0);
        // int vs float (mixed numeric)
        integer c = (1 == 1.0) + (1 != 2.5) + (1 < 1.5) + (2 > 1.5) + (1 <= 1.0) + (1 >= 1.0);
        // string vs string  (only == and != are meaningful, but < > work as integer 0)
        integer d = ("a" == "a") + ("a" != "b");
        // key vs key
        integer e = (NULL_KEY == NULL_KEY) + (NULL_KEY != (key)"00000000-0000-0000-0000-000000000001");
        // vector vs vector
        integer f = (<1.0,2.0,3.0> == <1.0,2.0,3.0>) + (<1.0,2.0,3.0> != <4.0,5.0,6.0>);
        // rotation vs rotation
        integer g = (<0.0,0.0,0.0,1.0> == <0.0,0.0,0.0,1.0>);
        // list compares by length
        integer h = ([1,2] == [3,4]) + ([1] != [1,2]);
        llOwnerSay((string)(a+b+c+d+e+f+g+h));
    }
}
