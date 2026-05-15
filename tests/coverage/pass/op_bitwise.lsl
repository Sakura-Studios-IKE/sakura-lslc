// Feature: bitwise & | ^ ~ << >> on integers.
// Expected: PASS
default
{
    state_entry()
    {
        integer a = 0xFF;
        integer b = 0x0F;
        integer and_ = a & b;
        integer or_  = a | b;
        integer xor_ = a ^ b;
        integer not_ = ~a;
        integer shl  = 1 << 4;
        integer shr  = 256 >> 2;
        // re-assigned via explicit binop
        integer c = 0xFF;
        c = c & 0x0F;
        c = c | 0xF0;
        c = c ^ 0xFF;
        llOwnerSay("and=" + (string)and_ + " or=" + (string)or_ + " xor=" + (string)xor_ + " not=" + (string)not_ + " shl=" + (string)shl + " shr=" + (string)shr + " c=" + (string)c);
    }
}
