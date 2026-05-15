/* lexer.c - LSL tokenizer */
#include "lsl.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer *L, const char *file, const char *src, size_t len, DiagCtx *d) {
    L->src = src; L->len = len; L->off = 0;
    L->line = 1; L->col = 1;
    L->file = file; L->diag = d;
    /* Skip UTF-8 BOM if present */
    if (len >= 3 && (unsigned char)src[0] == 0xEF
                 && (unsigned char)src[1] == 0xBB
                 && (unsigned char)src[2] == 0xBF) {
        L->off = 3;
    }
}

static int peekc(Lexer *L, size_t k) {
    if (L->off + k >= L->len) return -1;
    return (unsigned char)L->src[L->off + k];
}
static int getc_(Lexer *L) {
    if (L->off >= L->len) return -1;
    int c = (unsigned char)L->src[L->off++];
    if (c == '\n') { L->line++; L->col = 1; }
    else if (c == '\t') { L->col += 1; }
    else L->col++;
    return c;
}

static SrcLoc here(Lexer *L) {
    SrcLoc l = {0};
    l.file = L->file; l.line = L->line; l.col = L->col; l.off = L->off;
    return l;
}

static int is_ident_start(int c) { return (c == '_' || isalpha(c)); }
static int is_ident_cont(int c)  { return (c == '_' || isalnum(c)); }

static int at_line_start(Lexer *L) {
    if (L->off == 0) return 1;
    return L->src[L->off - 1] == '\n';
}

/* Parse a `#line N "file"` marker (only when emitted by the preprocessor).
 * Updates L->line and L->file. Any other `#…` line is left for the diag to
 * surface (it shouldn't happen if the preprocessor ran). */
/* Returns 1 if it consumed a #line directive (and advanced L->off). */
static int try_line_marker(Lexer *L) {
    if (!at_line_start(L)) return 0;
    size_t save = L->off;
    if (peekc(L, 0) != '#') return 0;
    /* Confirm "line" follows */
    size_t i = L->off + 1;
    while (i < L->len && (L->src[i] == ' ' || L->src[i] == '\t')) i++;
    if (i + 4 > L->len || strncmp(L->src + i, "line", 4) != 0) { L->off = save; return 0; }
    i += 4;
    if (i >= L->len || (L->src[i] != ' ' && L->src[i] != '\t')) { L->off = save; return 0; }
    /* Parse number */
    while (i < L->len && (L->src[i] == ' ' || L->src[i] == '\t')) i++;
    int new_line = 0;
    while (i < L->len && L->src[i] >= '0' && L->src[i] <= '9') {
        new_line = new_line * 10 + (L->src[i] - '0'); i++;
    }
    while (i < L->len && (L->src[i] == ' ' || L->src[i] == '\t')) i++;
    /* Parse "file" if present. Each unique path is interned in a static
     * table whose lifetime spans the compiler run so SrcLoc.file pointers
     * remain valid forever. */
    static struct { char *name; } interned[256];
    static int n_interned = 0;
    char tmpbuf[1024];
    int have_file = 0;
    if (i < L->len && L->src[i] == '"') {
        i++; size_t fs = i;
        while (i < L->len && L->src[i] != '"' && L->src[i] != '\n') i++;
        size_t flen = i - fs;
        if (flen < sizeof tmpbuf) { memcpy(tmpbuf, L->src + fs, flen); tmpbuf[flen] = '\0'; have_file = 1; }
        if (i < L->len && L->src[i] == '"') i++;
    }
    /* Skip to end of line */
    while (i < L->len && L->src[i] != '\n') i++;
    if (i < L->len) i++;
    L->off = i;
    L->line = new_line;
    L->col = 1;
    if (have_file) {
        const char *existing = NULL;
        for (int k = 0; k < n_interned; k++)
            if (strcmp(interned[k].name, tmpbuf) == 0) { existing = interned[k].name; break; }
        if (!existing && n_interned < (int)(sizeof interned / sizeof interned[0])) {
            interned[n_interned].name = xstrdup(tmpbuf);
            existing = interned[n_interned].name;
            n_interned++;
        }
        if (existing) L->file = existing;
    }
    return 1;
}

static void skip_whitespace_and_comments(Lexer *L) {
    for (;;) {
        if (try_line_marker(L)) continue;
        int c = peekc(L, 0);
        if (c == -1) return;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { getc_(L); continue; }
        if (c == '/' && peekc(L, 1) == '/') {
            while (peekc(L, 0) != -1 && peekc(L, 0) != '\n') getc_(L);
            continue;
        }
        if (c == '/' && peekc(L, 1) == '*') {
            SrcLoc start = here(L);
            getc_(L); getc_(L);
            int closed = 0;
            while (peekc(L, 0) != -1) {
                if (peekc(L, 0) == '*' && peekc(L, 1) == '/') {
                    getc_(L); getc_(L); closed = 1; break;
                }
                getc_(L);
            }
            if (!closed) {
                diag_emit(L->diag, DIAG_ERROR, start,
                          "unterminated /* */ comment");
                diag_hint(L->diag, "add */ to close this block comment");
            }
            continue;
        }
        return;
    }
}

typedef struct { const char *s; TokKind k; } Kw;
static const Kw KEYWORDS[] = {
    {"integer",   TK_KW_INTEGER},
    {"float",     TK_KW_FLOAT},
    {"string",    TK_KW_STRING},
    {"key",       TK_KW_KEY},
    {"vector",    TK_KW_VECTOR},
    {"rotation",  TK_KW_ROTATION},
    {"quaternion",TK_KW_QUATERNION},
    {"list",      TK_KW_LIST},
    {"default",   TK_KW_DEFAULT},
    {"state",     TK_KW_STATE},
    {"if",        TK_KW_IF},
    {"else",      TK_KW_ELSE},
    {"while",     TK_KW_WHILE},
    {"do",        TK_KW_DO},
    {"for",       TK_KW_FOR},
    {"return",    TK_KW_RETURN},
    {"jump",      TK_KW_JUMP},
    {"print",     TK_KW_PRINT},
    {NULL, TK_EOF}
};

static TokKind classify_ident(const char *s, size_t n) {
    for (int i = 0; KEYWORDS[i].s; i++) {
        if (strlen(KEYWORDS[i].s) == n && strncmp(KEYWORDS[i].s, s, n) == 0)
            return KEYWORDS[i].k;
    }
    return TK_IDENT;
}

const char *tok_kind_name(TokKind k) {
    switch (k) {
        case TK_EOF: return "EOF";
        case TK_IDENT: return "identifier";
        case TK_INT: return "integer-literal";
        case TK_FLOAT: return "float-literal";
        case TK_STRING: return "string-literal";
        case TK_KW_INTEGER: return "'integer'";
        case TK_KW_FLOAT: return "'float'";
        case TK_KW_STRING: return "'string'";
        case TK_KW_KEY: return "'key'";
        case TK_KW_VECTOR: return "'vector'";
        case TK_KW_ROTATION: return "'rotation'";
        case TK_KW_QUATERNION: return "'quaternion'";
        case TK_KW_LIST: return "'list'";
        case TK_KW_DEFAULT: return "'default'";
        case TK_KW_STATE: return "'state'";
        case TK_KW_IF: return "'if'";
        case TK_KW_ELSE: return "'else'";
        case TK_KW_WHILE: return "'while'";
        case TK_KW_DO: return "'do'";
        case TK_KW_FOR: return "'for'";
        case TK_KW_RETURN: return "'return'";
        case TK_KW_JUMP: return "'jump'";
        case TK_KW_PRINT: return "'print'";
        case TK_LPAREN: return "'('";
        case TK_RPAREN: return "')'";
        case TK_LBRACE: return "'{'";
        case TK_RBRACE: return "'}'";
        case TK_LBRACK: return "'['";
        case TK_RBRACK: return "']'";
        case TK_LANGLE: return "'<'";
        case TK_RANGLE: return "'>'";
        case TK_SEMI: return "';'";
        case TK_COMMA: return "','";
        case TK_AT: return "'@'";
        case TK_COLON: return "':'";
        case TK_DOT: return "'.'";
        case TK_ASSIGN: return "'='";
        case TK_PLUS_ASSIGN: return "'+='";
        case TK_MINUS_ASSIGN: return "'-='";
        case TK_STAR_ASSIGN: return "'*='";
        case TK_SLASH_ASSIGN: return "'/='";
        case TK_PERCENT_ASSIGN: return "'%='";
        case TK_PLUS: return "'+'";
        case TK_MINUS: return "'-'";
        case TK_STAR: return "'*'";
        case TK_SLASH: return "'/'";
        case TK_PERCENT: return "'%'";
        case TK_AND: return "'&'";
        case TK_OR:  return "'|'";
        case TK_XOR: return "'^'";
        case TK_TILDE: return "'~'";
        case TK_LAND: return "'&&'";
        case TK_LOR:  return "'||'";
        case TK_NOT:  return "'!'";
        case TK_EQ: return "'=='";
        case TK_NEQ: return "'!='";
        case TK_LT: return "'<'";
        case TK_GT: return "'>'";
        case TK_LE: return "'<='";
        case TK_GE: return "'>='";
        case TK_SHL: return "'<<'";
        case TK_SHR: return "'>>'";
        case TK_INC: return "'++'";
        case TK_DEC: return "'--'";
    }
    return "<token>";
}
const char *tok_kind_display(TokKind k) { return tok_kind_name(k); }

void token_free(Token *t) {
    if (!t) return;
    if (t->kind == TK_IDENT || t->kind == TK_STRING) {
        free(t->v.s); t->v.s = NULL;
    }
}

static int read_hex_digit(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static Token lex_number(Lexer *L) {
    Token t = {0};
    t.loc = here(L);
    size_t start = L->off;

    int is_float = 0;
    int is_hex = 0;
    int c = peekc(L, 0);
    if (c == '0' && (peekc(L, 1) == 'x' || peekc(L, 1) == 'X')) {
        is_hex = 1;
        getc_(L); getc_(L);
        while (read_hex_digit(peekc(L, 0)) >= 0) getc_(L);
    } else {
        while (isdigit(peekc(L, 0))) getc_(L);
        if (peekc(L, 0) == '.') {
            is_float = 1;
            getc_(L);
            while (isdigit(peekc(L, 0))) getc_(L);
        }
        if (peekc(L, 0) == 'e' || peekc(L, 0) == 'E') {
            is_float = 1;
            getc_(L);
            if (peekc(L, 0) == '+' || peekc(L, 0) == '-') getc_(L);
            while (isdigit(peekc(L, 0))) getc_(L);
        }
        if (peekc(L, 0) == 'f' || peekc(L, 0) == 'F') {
            is_float = 1;
            getc_(L);  /* consume float suffix */
        }
    }
    size_t end = L->off;
    t.end_off = end;

    char *buf = xstrndup(L->src + start, end - start);
    /* Strip trailing 'f' suffix for parsing. */
    if (is_float) {
        size_t bl = strlen(buf);
        if (bl && (buf[bl-1] == 'f' || buf[bl-1] == 'F')) buf[bl-1] = '\0';
        t.kind = TK_FLOAT;
        t.v.f = strtod(buf, NULL);
    } else if (is_hex) {
        t.kind = TK_INT;
        t.v.i = (long long)strtoull(buf, NULL, 16);
    } else {
        t.kind = TK_INT;
        t.v.i = strtoll(buf, NULL, 10);
    }
    free(buf);
    return t;
}

static Token lex_string(Lexer *L) {
    Token t = {0};
    t.loc = here(L);
    SrcLoc start = t.loc;
    getc_(L);  /* consume opening " */

    size_t cap = 32, n = 0;
    char *buf = (char*)xmalloc(cap);
    int unterminated = 0;
    int c;
    for (;;) {
        c = peekc(L, 0);
        if (c == -1) { unterminated = 1; break; }
        if (c == '"') { getc_(L); break; }
        if (c == '\n') {
            /* LSL allows multiline strings; newline becomes literal \n. */
            getc_(L);
            if (n + 1 >= cap) { cap *= 2; buf = (char*)xrealloc(buf, cap); }
            buf[n++] = '\n';
            continue;
        }
        if (c == '\\') {
            getc_(L);
            int e = peekc(L, 0);
            int decoded = -1;
            switch (e) {
                case '"':  decoded = '"';  getc_(L); break;
                case '\\': decoded = '\\'; getc_(L); break;
                case 'n':  decoded = '\n'; getc_(L); break;
                case 't':  decoded = '\t'; getc_(L); break;
                case '\n': getc_(L); continue;
                default:
                    /* In LSL, an unknown escape is literally kept as a
                     * backslash followed by the character (matches the SL
                     * compiler behaviour). */
                    if (e == -1) { unterminated = 1; }
                    else {
                        if (n + 2 >= cap) { cap *= 2; buf = (char*)xrealloc(buf, cap); }
                        buf[n++] = '\\';
                        buf[n++] = (char)e;
                        getc_(L);
                    }
                    decoded = -1;
                    break;
            }
            if (decoded >= 0) {
                if (n + 1 >= cap) { cap *= 2; buf = (char*)xrealloc(buf, cap); }
                buf[n++] = (char)decoded;
            }
            if (unterminated) break;
            continue;
        }
        if (n + 1 >= cap) { cap *= 2; buf = (char*)xrealloc(buf, cap); }
        buf[n++] = (char)c;
        getc_(L);
    }
    buf[n] = '\0';
    t.kind = TK_STRING;
    t.v.s = buf;
    t.end_off = L->off;
    if (unterminated) {
        diag_emit_range(L->diag, DIAG_ERROR, start, L->off,
                        "unterminated string literal");
        diag_hint(L->diag,
            "string literals must be closed with a matching \" — did you forget one?");
    }
    return t;
}

static Token lex_ident(Lexer *L) {
    Token t = {0};
    t.loc = here(L);
    size_t start = L->off;
    while (is_ident_cont(peekc(L, 0))) getc_(L);
    size_t end = L->off;
    t.end_off = end;
    t.kind = classify_ident(L->src + start, end - start);
    if (t.kind == TK_IDENT) {
        t.v.s = xstrndup(L->src + start, end - start);
    }
    return t;
}

static Token simple(Lexer *L, TokKind k, size_t consume) {
    Token t = {0};
    t.loc = here(L);
    for (size_t i = 0; i < consume; i++) getc_(L);
    t.kind = k;
    t.end_off = L->off;
    return t;
}

Token lexer_next(Lexer *L) {
    skip_whitespace_and_comments(L);
    Token t = {0};
    if (L->off >= L->len) {
        t.loc = here(L);
        t.kind = TK_EOF;
        t.end_off = L->off;
        return t;
    }
    int c = peekc(L, 0);
    int c1 = peekc(L, 1);
    if (is_ident_start(c)) return lex_ident(L);
    if (isdigit(c)) return lex_number(L);
    if (c == '.' && isdigit(c1)) return lex_number(L);
    if (c == '"') return lex_string(L);
    switch (c) {
        case '(': return simple(L, TK_LPAREN, 1);
        case ')': return simple(L, TK_RPAREN, 1);
        case '{': return simple(L, TK_LBRACE, 1);
        case '}': return simple(L, TK_RBRACE, 1);
        case '[': return simple(L, TK_LBRACK, 1);
        case ']': return simple(L, TK_RBRACK, 1);
        case ';': return simple(L, TK_SEMI, 1);
        case ',': return simple(L, TK_COMMA, 1);
        case '@': return simple(L, TK_AT, 1);
        case ':': return simple(L, TK_COLON, 1);
        case '.': return simple(L, TK_DOT, 1);
        case '~': return simple(L, TK_TILDE, 1);
        case '?': {
            SrcLoc loc = here(L);
            simple(L, TK_NOT, 1);
            diag_emit(L->diag, DIAG_ERROR, loc,
                      "'?' is not an LSL operator (no ternary in LSL)");
            diag_hint(L->diag,
                "rewrite the conditional expression using if/else statements");
            return lexer_next(L);
        }
        case '+':
            if (c1 == '+') return simple(L, TK_INC, 2);
            if (c1 == '=') return simple(L, TK_PLUS_ASSIGN, 2);
            return simple(L, TK_PLUS, 1);
        case '-':
            if (c1 == '-') return simple(L, TK_DEC, 2);
            if (c1 == '=') return simple(L, TK_MINUS_ASSIGN, 2);
            return simple(L, TK_MINUS, 1);
        case '*':
            if (c1 == '=') return simple(L, TK_STAR_ASSIGN, 2);
            return simple(L, TK_STAR, 1);
        case '/':
            if (c1 == '=') return simple(L, TK_SLASH_ASSIGN, 2);
            return simple(L, TK_SLASH, 1);
        case '%':
            if (c1 == '=') return simple(L, TK_PERCENT_ASSIGN, 2);
            return simple(L, TK_PERCENT, 1);
        case '&':
            if (c1 == '&') return simple(L, TK_LAND, 2);
            return simple(L, TK_AND, 1);
        case '|':
            if (c1 == '|') return simple(L, TK_LOR, 2);
            return simple(L, TK_OR, 1);
        case '^': return simple(L, TK_XOR, 1);
        case '!':
            if (c1 == '=') return simple(L, TK_NEQ, 2);
            return simple(L, TK_NOT, 1);
        case '=':
            if (c1 == '=') return simple(L, TK_EQ, 2);
            return simple(L, TK_ASSIGN, 1);
        case '<':
            if (c1 == '=') return simple(L, TK_LE, 2);
            if (c1 == '<') return simple(L, TK_SHL, 2);
            return simple(L, TK_LT, 1);
        case '>':
            if (c1 == '=') return simple(L, TK_GE, 2);
            if (c1 == '>') return simple(L, TK_SHR, 2);
            return simple(L, TK_GT, 1);
        default: {
            SrcLoc loc = here(L);
            int ch = c;
            getc_(L);
            char buf[8];
            if (ch >= 32 && ch < 127)
                snprintf(buf, sizeof buf, "'%c'", ch);
            else
                snprintf(buf, sizeof buf, "0x%02X", ch & 0xFF);
            diag_emit(L->diag, DIAG_ERROR, loc,
                      "unexpected character %s in source", buf);
            return lexer_next(L);
        }
    }
}
