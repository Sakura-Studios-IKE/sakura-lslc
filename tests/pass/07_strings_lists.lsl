// Exercise a wide range of LSL string and list built-ins.
default
{
    state_entry()
    {
        string s = "  Hello, World!  ";
        string trimmed = llStringTrim(s, STRING_TRIM);
        string lower   = llToLower(trimmed);
        string upper   = llToUpper(trimmed);
        integer len    = llStringLength(trimmed);
        integer idx    = llSubStringIndex(trimmed, "World");
        string  piece  = llGetSubString(trimmed, 0, 4);
        string  del    = llDeleteSubString(trimmed, 0, 5);

        list a = [1, 2, 3];
        list b = a + [4, 5, 6];
        list c = llListInsertList(a, [99], 1);
        list d = llListReplaceList(a, [0], 1, 1);
        list e = llListSort(b, 1, TRUE);
        integer find = llListFindList(b, [3]);
        integer ln   = llGetListLength(b);
        integer v0   = llList2Integer(b, 0);
        string v0s   = llList2String(b, 0);
        float   v0f  = llList2Float(b, 0);
        key     v0k  = llList2Key([NULL_KEY], 0);

        list parts = llParseString2List("a,b,c,d", [","], []);
        string csv = llList2CSV(parts);
        string dump = llDumpList2String(parts, "|");

        llOwnerSay(lower + " / " + upper + " idx=" + (string)idx
                   + " len=" + (string)len + " piece=" + piece + " del=" + del
                   + " find=" + (string)find + " v0=" + (string)v0
                   + " csv=" + csv + " dump=" + dump);
    }
}
