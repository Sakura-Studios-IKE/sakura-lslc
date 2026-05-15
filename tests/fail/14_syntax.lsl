// Syntax errors: missing ';', unclosed comment, unterminated string,
// mismatched braces, bad for-header.
default
{
    state_entry()
    {
        integer x = 1            // <-- missing semicolon
        string s = "unterminated
        for (i = 0 i < 10; i++)  // missing first ';'
        {
        }
        /* unterminated comment ...
    }
