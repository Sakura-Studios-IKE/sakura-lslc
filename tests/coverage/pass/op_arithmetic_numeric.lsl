// Feature: numeric arithmetic — int+int, int+float, float+float, with - * / %.
// Expected: PASS
default
{
    state_entry()
    {
        integer ii = 1 + 2 - 3 * 4 / 2 % 5;
        // int + float: result is float
        float   if_ = 1 + 2.0;
        float   fi  = 2.5 + 1;
        // float + float
        float   ff  = 1.5 + 2.5 - 0.5 * 2.0 / 1.0;
        // % is integer only for non-vector cases
        integer mod = 7 % 3;
        llOwnerSay("ii=" + (string)ii + " if=" + (string)if_ + " fi=" + (string)fi + " ff=" + (string)ff + " mod=" + (string)mod);
    }
}
