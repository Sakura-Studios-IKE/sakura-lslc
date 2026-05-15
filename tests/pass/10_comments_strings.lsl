// Various comment forms and string escapes.
/* This is a
   multi-line comment. */
string oddly = "Line1\nLine2\tindented\\ and a quote: \" end";

default
{
    /* event-prefix comment */
    state_entry()
    {
        // line comment inside event
        llOwnerSay(oddly);
        /* nested example: not actually nested but with stars * inside */
        integer x = /* mid-expression */ 5 /* trailing */ ;
        llOwnerSay("x=" + (string)x);
    }
}
