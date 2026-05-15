/* parser.c - recursive descent parser for LSL */
#include "lsl.h"
#include <stdlib.h>
#include <string.h>

/* --------------------------- Helper macros ------------------------------ */

static Expr *mk_expr(ExprKind k, SrcLoc loc) {
    Expr *e = (Expr*)xcalloc(1, sizeof(Expr));
    e->kind = k; e->loc = loc; e->end_off = loc.off + 1;
    e->type = T_ERROR; return e;
}
static Stmt *mk_stmt(StmtKind k, SrcLoc loc) {
    Stmt *s = (Stmt*)xcalloc(1, sizeof(Stmt));
    s->kind = k; s->loc = loc; return s;
}

/* ------------------------------- I/O ------------------------------------ */

static Token take_tok(Parser *p) {
    Token t;
    if (p->has_peek) { t = p->peek; p->has_peek = 0; }
    else { t = lexer_next(p->lex); }
    return t;
}

static void prime(Parser *p) {
    p->cur = lexer_next(p->lex);
    p->peek = lexer_next(p->lex);
    p->has_peek = 1;
}
static void advance(Parser *p) {
    token_free(&p->cur);
    p->cur = p->peek;
    p->peek = lexer_next(p->lex);
    p->has_peek = 1;
}

static int at(Parser *p, TokKind k) { return p->cur.kind == k; }
static int eat(Parser *p, TokKind k) {
    if (p->cur.kind == k) { advance(p); return 1; }
    return 0;
}

static const char *cur_lexeme(Parser *p) {
    static char buf[64];
    if (p->cur.kind == TK_IDENT && p->cur.v.s) return p->cur.v.s;
    return tok_kind_name(p->cur.kind);
}

static int expect(Parser *p, TokKind k, const char *hint) {
    if (eat(p, k)) return 1;
    diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
              "expected %s but found %s", tok_kind_name(k), cur_lexeme(p));
    if (hint) diag_hint(p->diag, "%s", hint);
    return 0;
}

/* --------------------------- Forward decls ------------------------------ */

static Expr *parse_expr(Parser *p);
static Expr *parse_assign(Parser *p);
static Expr *parse_logical_or(Parser *p);
static Expr *parse_logical_and(Parser *p);
static Expr *parse_bit_or(Parser *p);
static Expr *parse_bit_xor(Parser *p);
static Expr *parse_bit_and(Parser *p);
static Expr *parse_eq(Parser *p);
static Expr *parse_rel(Parser *p);
static Expr *parse_shift(Parser *p);
static Expr *parse_addsub(Parser *p);
static Expr *parse_muldiv(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_cast_or_primary(Parser *p);
static Expr *parse_postfix(Parser *p, Expr *base);
static Expr *parse_primary(Parser *p);
static Expr *parse_vector_or_rotation(Parser *p);
static Expr *parse_list_literal(Parser *p);

/* Used inside vector components: lower precedence than comparisons. */
static Expr *parse_addsub_for_vector(Parser *p);

static Stmt *parse_stmt(Parser *p);
static Stmt *parse_block(Parser *p);

/* --------------------------- Type keywords ------------------------------ */

static int is_type_kw(TokKind k) {
    switch (k) {
        case TK_KW_INTEGER:
        case TK_KW_FLOAT:
        case TK_KW_STRING:
        case TK_KW_KEY:
        case TK_KW_VECTOR:
        case TK_KW_ROTATION:
        case TK_KW_QUATERNION:
        case TK_KW_LIST:
            return 1;
        default: return 0;
    }
}
static TypeKind tk_to_type(TokKind k) {
    switch (k) {
        case TK_KW_INTEGER:   return T_INTEGER;
        case TK_KW_FLOAT:     return T_FLOAT;
        case TK_KW_STRING:    return T_STRING;
        case TK_KW_KEY:       return T_KEY;
        case TK_KW_VECTOR:    return T_VECTOR;
        case TK_KW_ROTATION:  return T_ROTATION;
        case TK_KW_QUATERNION:return T_ROTATION;
        case TK_KW_LIST:      return T_LIST;
        default: return T_ERROR;
    }
}

/* --------------------------- Expression parser -------------------------- */

static Expr *parse_primary(Parser *p) {
    SrcLoc loc = p->cur.loc;
    if (at(p, TK_INT)) {
        Expr *e = mk_expr(E_INT_LIT, loc);
        e->u.ilit = p->cur.v.i;
        e->end_off = p->cur.end_off;
        e->type = T_INTEGER;
        advance(p);
        return e;
    }
    if (at(p, TK_FLOAT)) {
        Expr *e = mk_expr(E_FLOAT_LIT, loc);
        e->u.flit = p->cur.v.f;
        e->end_off = p->cur.end_off;
        e->type = T_FLOAT;
        advance(p);
        return e;
    }
    if (at(p, TK_STRING)) {
        Expr *e = mk_expr(E_STRING_LIT, loc);
        e->u.slit = p->cur.v.s;  /* take ownership */
        p->cur.v.s = NULL;
        e->end_off = p->cur.end_off;
        e->type = T_STRING;
        advance(p);
        return e;
    }
    if (at(p, TK_IDENT)) {
        Expr *e = mk_expr(E_IDENT, loc);
        e->u.ident = p->cur.v.s;
        p->cur.v.s = NULL;
        e->end_off = p->cur.end_off;
        advance(p);
        return e;
    }
    if (eat(p, TK_LPAREN)) {
        Expr *e = parse_expr(p);
        expect(p, TK_RPAREN, "matching ')'");
        return e;
    }
    if (at(p, TK_LT)) {
        return parse_vector_or_rotation(p);
    }
    if (at(p, TK_LBRACK)) {
        return parse_list_literal(p);
    }
    diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
              "expected expression, found %s", cur_lexeme(p));
    diag_hint(p->diag, "an expression is an identifier, literal, parenthesised expression, vector/rotation literal <x,y,z[,w]>, or list literal [..]");
    Expr *e = mk_expr(E_INT_LIT, p->cur.loc);
    e->u.ilit = 0; e->type = T_ERROR;
    advance(p);
    return e;
}

static Expr *parse_vector_or_rotation(Parser *p) {
    SrcLoc loc = p->cur.loc;
    eat(p, TK_LT);
    Expr *x = parse_addsub_for_vector(p);
    expect(p, TK_COMMA, "vectors and rotations need 3 or 4 comma-separated components");
    Expr *y = parse_addsub_for_vector(p);
    expect(p, TK_COMMA, "vectors and rotations need 3 or 4 comma-separated components");
    Expr *z = parse_addsub_for_vector(p);
    Expr *w = NULL;
    int is_rot = 0;
    if (eat(p, TK_COMMA)) {
        is_rot = 1;
        w = parse_addsub_for_vector(p);
    }
    /* Closing '>' may have merged with following token in lexer in tricky
     * cases. We accept TK_RANGLE / TK_GT and also TK_GE (split into '>' '=')
     * if the lexer is misled by '>='. */
    SrcLoc end = p->cur.loc;
    size_t end_off = p->cur.end_off;
    if (!at(p, TK_GT)) {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                  "expected '>' to close the %s literal",
                  is_rot ? "rotation" : "vector");
        diag_hint(p->diag, "vectors are written as <x, y, z>, rotations as <x, y, z, s>");
    } else {
        end_off = p->cur.end_off;
        advance(p);
    }
    Expr *e = mk_expr(is_rot ? E_ROT_LIT : E_VECTOR_LIT, loc);
    e->u.vec.x = x; e->u.vec.y = y; e->u.vec.z = z; e->u.vec.w = w;
    e->end_off = end_off;
    e->type = is_rot ? T_ROTATION : T_VECTOR;
    (void)end;
    return e;
}

static Expr *parse_list_literal(Parser *p) {
    SrcLoc loc = p->cur.loc;
    eat(p, TK_LBRACK);
    Expr **items = NULL;
    int n = 0, cap = 0;
    if (!at(p, TK_RBRACK)) {
        for (;;) {
            if (n == cap) { cap = cap ? cap * 2 : 4; items = (Expr**)xrealloc(items, sizeof(*items) * cap); }
            items[n++] = parse_assign(p);
            if (!eat(p, TK_COMMA)) break;
        }
    }
    SrcLoc end_loc = p->cur.loc;
    size_t end_off = p->cur.end_off;
    expect(p, TK_RBRACK, "list literals end with ']'");
    Expr *e = mk_expr(E_LIST_LIT, loc);
    e->u.list.items = items;
    e->u.list.n = n;
    e->end_off = end_off;
    e->type = T_LIST;
    (void)end_loc;
    return e;
}

static Expr *parse_postfix(Parser *p, Expr *base) {
    for (;;) {
        if (at(p, TK_LPAREN) && base->kind == E_IDENT) {
            /* function call */
            SrcLoc cloc = base->loc;
            advance(p);  /* ( */
            Expr **args = NULL;
            int n = 0, cap = 0;
            if (!at(p, TK_RPAREN)) {
                for (;;) {
                    if (n == cap) { cap = cap ? cap * 2 : 4; args = (Expr**)xrealloc(args, sizeof(*args) * cap); }
                    args[n++] = parse_assign(p);
                    if (!eat(p, TK_COMMA)) break;
                }
            }
            size_t end_off = p->cur.end_off;
            expect(p, TK_RPAREN, "function call argument list must end with ')'");
            Expr *call = mk_expr(E_CALL, cloc);
            call->u.call.name = base->u.ident; base->u.ident = NULL;
            call->u.call.args = args; call->u.call.n_args = n;
            call->end_off = end_off;
            ast_free_expr(base);
            base = call;
            continue;
        }
        if (eat(p, TK_DOT)) {
            if (!at(p, TK_IDENT)) {
                diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                          "expected member name after '.'");
                diag_hint(p->diag, "vectors have .x .y .z components; rotations also have .s");
                return base;
            }
            char *id = p->cur.v.s; p->cur.v.s = NULL;
            SrcLoc mloc = p->cur.loc;
            size_t end_off = p->cur.end_off;
            advance(p);
            char m = 0;
            if (id && id[0] && id[1] == '\0') m = id[0];
            else {
                diag_emit(p->diag, DIAG_ERROR, mloc,
                          "invalid member '%s'", id);
                diag_hint(p->diag, "vector/rotation members must be a single letter x, y, z (or s for rotation)");
            }
            Expr *me = mk_expr(E_MEMBER, base->loc);
            me->u.memb.base = base;
            me->u.memb.member = m;
            me->end_off = end_off;
            free(id);
            base = me;
            continue;
        }
        if (eat(p, TK_INC)) {
            Expr *pe = mk_expr(E_POSTFIX, base->loc);
            pe->u.post.op = TK_INC; pe->u.post.inner = base;
            base = pe; continue;
        }
        if (eat(p, TK_DEC)) {
            Expr *pe = mk_expr(E_POSTFIX, base->loc);
            pe->u.post.op = TK_DEC; pe->u.post.inner = base;
            base = pe; continue;
        }
        break;
    }
    return base;
}

/* Distinguish (TYPE) cast vs parenthesised expression. */
static Expr *parse_cast_or_primary(Parser *p) {
    if (at(p, TK_LPAREN) && is_type_kw(p->peek.kind)) {
        SrcLoc loc = p->cur.loc;
        advance(p);   /* ( */
        TypeKind t = tk_to_type(p->cur.kind);
        advance(p);   /* type kw */
        expect(p, TK_RPAREN, "missing ')' after cast type");
        Expr *inner = parse_unary(p);
        Expr *e = mk_expr(E_CAST, loc);
        e->u.cast.to = t;
        e->u.cast.inner = inner;
        e->type = t;
        return e;
    }
    Expr *prim = parse_primary(p);
    return parse_postfix(p, prim);
}

static Expr *parse_unary(Parser *p) {
    SrcLoc loc = p->cur.loc;
    if (at(p, TK_MINUS) || at(p, TK_PLUS) || at(p, TK_NOT) || at(p, TK_TILDE)) {
        TokKind op = p->cur.kind;
        advance(p);
        Expr *inner = parse_unary(p);
        Expr *e = mk_expr(E_UNARY, loc);
        e->u.un.op = op; e->u.un.inner = inner;
        return e;
    }
    if (at(p, TK_INC) || at(p, TK_DEC)) {
        TokKind op = p->cur.kind;
        advance(p);
        Expr *inner = parse_unary(p);
        Expr *e = mk_expr(E_UNARY, loc);
        e->u.un.op = op; e->u.un.inner = inner;
        return e;
    }
    return parse_cast_or_primary(p);
}

static Expr *bin(SrcLoc loc, TokKind op, Expr *l, Expr *r) {
    Expr *e = mk_expr(E_BINARY, loc);
    e->u.bin.op = op; e->u.bin.l = l; e->u.bin.r = r;
    return e;
}
static Expr *parse_muldiv(Parser *p) {
    Expr *l = parse_unary(p);
    while (at(p, TK_STAR) || at(p, TK_SLASH) || at(p, TK_PERCENT)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_unary(p);
        l = bin(loc, op, l, r);
    }
    return l;
}
static Expr *parse_addsub(Parser *p) {
    Expr *l = parse_muldiv(p);
    while (at(p, TK_PLUS) || at(p, TK_MINUS)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_muldiv(p);
        l = bin(loc, op, l, r);
    }
    return l;
}
static Expr *parse_addsub_for_vector(Parser *p) {
    /* Inside vector literals, only allow up to shift level so that comma
     * and '>' remain reliable terminators. We disallow comparison operators
     * because '>' would otherwise be ambiguous with the closing bracket. */
    Expr *l = parse_addsub(p);
    while (at(p, TK_SHL) || at(p, TK_SHR)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_addsub(p);
        l = bin(loc, op, l, r);
    }
    return l;
}
static Expr *parse_shift(Parser *p) {
    Expr *l = parse_addsub(p);
    while (at(p, TK_SHL) || at(p, TK_SHR)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_addsub(p);
        l = bin(loc, op, l, r);
    }
    return l;
}
static Expr *parse_rel(Parser *p) {
    Expr *l = parse_shift(p);
    while (at(p, TK_LT) || at(p, TK_GT) || at(p, TK_LE) || at(p, TK_GE)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_shift(p);
        l = bin(loc, op, l, r);
    }
    return l;
}
static Expr *parse_eq(Parser *p) {
    Expr *l = parse_rel(p);
    while (at(p, TK_EQ) || at(p, TK_NEQ)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_rel(p);
        l = bin(loc, op, l, r);
    }
    return l;
}
static Expr *parse_bit_and(Parser *p) {
    Expr *l = parse_eq(p);
    while (at(p, TK_AND)) {
        SrcLoc loc = p->cur.loc; advance(p);
        Expr *r = parse_eq(p);
        l = bin(loc, TK_AND, l, r);
    }
    return l;
}
static Expr *parse_bit_xor(Parser *p) {
    Expr *l = parse_bit_and(p);
    while (at(p, TK_XOR)) {
        SrcLoc loc = p->cur.loc; advance(p);
        Expr *r = parse_bit_and(p);
        l = bin(loc, TK_XOR, l, r);
    }
    return l;
}
static Expr *parse_bit_or(Parser *p) {
    Expr *l = parse_bit_xor(p);
    while (at(p, TK_OR)) {
        SrcLoc loc = p->cur.loc; advance(p);
        Expr *r = parse_bit_xor(p);
        l = bin(loc, TK_OR, l, r);
    }
    return l;
}
static Expr *parse_logical_and(Parser *p) {
    Expr *l = parse_bit_or(p);
    while (at(p, TK_LAND)) {
        SrcLoc loc = p->cur.loc; advance(p);
        Expr *r = parse_bit_or(p);
        l = bin(loc, TK_LAND, l, r);
    }
    return l;
}
static Expr *parse_logical_or(Parser *p) {
    Expr *l = parse_logical_and(p);
    while (at(p, TK_LOR)) {
        SrcLoc loc = p->cur.loc; advance(p);
        Expr *r = parse_logical_and(p);
        l = bin(loc, TK_LOR, l, r);
    }
    return l;
}

static int is_assign_op(TokKind k) {
    switch (k) {
        case TK_ASSIGN: case TK_PLUS_ASSIGN: case TK_MINUS_ASSIGN:
        case TK_STAR_ASSIGN: case TK_SLASH_ASSIGN: case TK_PERCENT_ASSIGN:
            return 1;
        default: return 0;
    }
}

static Expr *parse_assign(Parser *p) {
    Expr *l = parse_logical_or(p);
    if (is_assign_op(p->cur.kind)) {
        SrcLoc loc = p->cur.loc; TokKind op = p->cur.kind;
        advance(p);
        Expr *r = parse_assign(p);
        Expr *e = mk_expr(E_ASSIGN, loc);
        e->u.asn.op = op; e->u.asn.l = l; e->u.asn.r = r;
        return e;
    }
    return l;
}

static Expr *parse_expr(Parser *p) { return parse_assign(p); }

/* --------------------------- Statements --------------------------------- */

static Stmt *parse_block(Parser *p) {
    SrcLoc loc = p->cur.loc;
    Stmt *blk = mk_stmt(S_BLOCK, loc);
    expect(p, TK_LBRACE, "blocks start with '{'");
    Stmt **arr = NULL; int n = 0, cap = 0;
    while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
        Stmt *s = parse_stmt(p);
        if (!s) continue;
        if (n == cap) { cap = cap ? cap * 2 : 8; arr = (Stmt**)xrealloc(arr, sizeof(*arr) * cap); }
        arr[n++] = s;
    }
    expect(p, TK_RBRACE, "matching '}'");
    blk->u.block.stmts = arr; blk->u.block.n = n;
    return blk;
}

static Stmt *parse_local_decl(Parser *p) {
    SrcLoc loc = p->cur.loc;
    TypeKind t = tk_to_type(p->cur.kind);
    advance(p);
    if (!at(p, TK_IDENT)) {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                  "expected a variable name after type '%s'", type_name(t));
        diag_hint(p->diag, "local declarations look like:  %s name;  or  %s name = expr;", type_name(t), type_name(t));
        /* Skip to ; for recovery */
        while (!at(p, TK_SEMI) && !at(p, TK_EOF)) advance(p);
        eat(p, TK_SEMI);
        Stmt *s = mk_stmt(S_EMPTY, loc); return s;
    }
    SrcLoc nloc = p->cur.loc;
    char *name = p->cur.v.s; p->cur.v.s = NULL;
    advance(p);
    Expr *init = NULL;
    if (eat(p, TK_ASSIGN)) {
        init = parse_assign(p);
    }
    expect(p, TK_SEMI, "declarations end with ';'");
    Stmt *s = mk_stmt(S_DECL, loc);
    s->u.decl.t = t; s->u.decl.name = name; s->u.decl.init = init;
    s->u.decl.name_loc = nloc;
    return s;
}

static Stmt *parse_if(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);  /* if */
    expect(p, TK_LPAREN, "'if' condition must be parenthesised: if (cond) ...");
    Expr *cond = parse_expr(p);
    expect(p, TK_RPAREN, "expected ')' to close 'if' condition");
    Stmt *th = parse_stmt(p);
    Stmt *el = NULL;
    if (eat(p, TK_KW_ELSE)) {
        el = parse_stmt(p);
    }
    Stmt *s = mk_stmt(S_IF, loc);
    s->u.if_s.cond = cond; s->u.if_s.then_s = th; s->u.if_s.else_s = el;
    return s;
}
static Stmt *parse_while(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);
    expect(p, TK_LPAREN, "'while' condition must be parenthesised");
    Expr *cond = parse_expr(p);
    expect(p, TK_RPAREN, "expected ')' to close 'while' condition");
    Stmt *body = parse_stmt(p);
    Stmt *s = mk_stmt(S_WHILE, loc);
    s->u.while_s.cond = cond; s->u.while_s.body = body;
    return s;
}
static Stmt *parse_do(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);
    Stmt *body = parse_stmt(p);
    expect(p, TK_KW_WHILE, "'do' must be followed by 'while (cond);'");
    expect(p, TK_LPAREN, "'while' after 'do' needs parenthesised condition");
    Expr *cond = parse_expr(p);
    expect(p, TK_RPAREN, "expected ')'");
    expect(p, TK_SEMI, "do-while ends with ';' after the condition");
    Stmt *s = mk_stmt(S_DO, loc);
    s->u.do_s.body = body; s->u.do_s.cond = cond;
    return s;
}
static Stmt *parse_for(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);
    expect(p, TK_LPAREN, "'for' uses C-style header: for (init; cond; step)");
    Expr **inits = NULL; int n_init = 0, ci = 0;
    if (!at(p, TK_SEMI)) {
        for (;;) {
            if (n_init == ci) { ci = ci ? ci * 2 : 2; inits = (Expr**)xrealloc(inits, sizeof(*inits) * ci); }
            inits[n_init++] = parse_assign(p);
            if (!eat(p, TK_COMMA)) break;
        }
    }
    expect(p, TK_SEMI, "expected ';' separating 'for' init from condition");
    Expr *cond = NULL;
    if (!at(p, TK_SEMI)) cond = parse_expr(p);
    expect(p, TK_SEMI, "expected ';' separating 'for' condition from step");
    Expr **posts = NULL; int n_post = 0, cp = 0;
    if (!at(p, TK_RPAREN)) {
        for (;;) {
            if (n_post == cp) { cp = cp ? cp * 2 : 2; posts = (Expr**)xrealloc(posts, sizeof(*posts) * cp); }
            posts[n_post++] = parse_assign(p);
            if (!eat(p, TK_COMMA)) break;
        }
    }
    expect(p, TK_RPAREN, "expected ')' closing the 'for' header");
    Stmt *body = parse_stmt(p);
    Stmt *s = mk_stmt(S_FOR, loc);
    s->u.for_s.init = inits; s->u.for_s.n_init = n_init;
    s->u.for_s.cond = cond;
    s->u.for_s.post = posts; s->u.for_s.n_post = n_post;
    s->u.for_s.body = body;
    return s;
}
static Stmt *parse_return(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);
    Stmt *s = mk_stmt(S_RETURN, loc);
    if (!at(p, TK_SEMI)) {
        s->u.ret.expr = parse_expr(p);
        s->u.ret.has_value = 1;
    }
    expect(p, TK_SEMI, "return statements end with ';'");
    return s;
}
static Stmt *parse_jump(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);
    if (!at(p, TK_IDENT)) {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc, "expected a label name after 'jump'");
        diag_hint(p->diag, "label syntax:  @my_label;   then  jump my_label;");
    }
    char *name = NULL;
    SrcLoc nl = p->cur.loc;
    if (at(p, TK_IDENT)) { name = p->cur.v.s; p->cur.v.s = NULL; advance(p); }
    expect(p, TK_SEMI, "jump statements end with ';'");
    Stmt *s = mk_stmt(S_JUMP, loc);
    s->u.jmp.label = name;
    s->u.jmp.lbl_loc = nl;
    return s;
}
static Stmt *parse_label(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);  /* @ */
    if (!at(p, TK_IDENT)) {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc, "expected a label name after '@'");
        diag_hint(p->diag, "labels are introduced with '@label_name;'");
    }
    char *name = NULL;
    if (at(p, TK_IDENT)) { name = p->cur.v.s; p->cur.v.s = NULL; advance(p); }
    expect(p, TK_SEMI, "labels end with ';'");
    Stmt *s = mk_stmt(S_LABEL, loc);
    s->u.lbl.label = name;
    return s;
}
static Stmt *parse_state_change(Parser *p) {
    SrcLoc loc = p->cur.loc;
    advance(p);  /* state */
    char *name = NULL;
    SrcLoc nloc = p->cur.loc;
    if (at(p, TK_KW_DEFAULT)) { name = xstrdup("default"); advance(p); }
    else if (at(p, TK_IDENT))  { name = p->cur.v.s; p->cur.v.s = NULL; advance(p); }
    else {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                  "expected a state name after 'state'");
        diag_hint(p->diag, "use  state default;  or  state your_state_name;");
    }
    expect(p, TK_SEMI, "state-change statements end with ';'");
    Stmt *s = mk_stmt(S_STATECHG, loc);
    s->u.stc.state = name;
    s->u.stc.name_loc = nloc;
    return s;
}

static Stmt *parse_stmt(Parser *p) {
    SrcLoc loc = p->cur.loc;
    if (at(p, TK_LBRACE)) return parse_block(p);
    if (at(p, TK_SEMI))   { advance(p); return mk_stmt(S_EMPTY, loc); }
    if (is_type_kw(p->cur.kind)) return parse_local_decl(p);
    if (at(p, TK_KW_IF))     return parse_if(p);
    if (at(p, TK_KW_WHILE))  return parse_while(p);
    if (at(p, TK_KW_DO))     return parse_do(p);
    if (at(p, TK_KW_FOR))    return parse_for(p);
    if (at(p, TK_KW_RETURN)) return parse_return(p);
    if (at(p, TK_KW_JUMP))   return parse_jump(p);
    if (at(p, TK_AT))        return parse_label(p);
    if (at(p, TK_KW_STATE))  return parse_state_change(p);
    if (at(p, TK_KW_PRINT)) {
        /* legacy debug print */
        advance(p);
        expect(p, TK_LPAREN, "print is a debug builtin: print(expr);");
        Expr *e = parse_expr(p);
        expect(p, TK_RPAREN, "expected ')'");
        expect(p, TK_SEMI, "expected ';'");
        Stmt *s = mk_stmt(S_EXPR, loc);
        Expr *call = mk_expr(E_CALL, loc);
        call->u.call.name = xstrdup("print");
        call->u.call.args = (Expr**)xmalloc(sizeof(Expr*));
        call->u.call.args[0] = e;
        call->u.call.n_args = 1;
        s->u.expr = call;
        return s;
    }

    /* expression statement */
    Expr *e = parse_expr(p);
    expect(p, TK_SEMI, "statements end with ';'");
    Stmt *s = mk_stmt(S_EXPR, loc);
    s->u.expr = e;
    return s;
}

/* --------------------------- Top-level ---------------------------------- */

static int parse_params(Parser *p, Param **out, int *n_out) {
    Param *arr = NULL; int n = 0, cap = 0;
    if (!at(p, TK_RPAREN)) {
        for (;;) {
            if (!is_type_kw(p->cur.kind)) {
                diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                          "expected parameter type (integer, float, string, key, vector, rotation, list)");
                diag_hint(p->diag, "every parameter must have an explicit type: %s name", "integer");
                break;
            }
            TypeKind t = tk_to_type(p->cur.kind);
            SrcLoc tloc = p->cur.loc;
            advance(p);
            if (!at(p, TK_IDENT)) {
                diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                          "expected parameter name after type '%s'", type_name(t));
                break;
            }
            if (n == cap) { cap = cap ? cap * 2 : 4; arr = (Param*)xrealloc(arr, sizeof(*arr) * cap); }
            arr[n].type = t;
            arr[n].name = p->cur.v.s; p->cur.v.s = NULL;
            arr[n].loc = tloc;
            n++;
            advance(p);
            if (!eat(p, TK_COMMA)) break;
        }
    }
    *out = arr; *n_out = n;
    return 1;
}

static int parse_event(Parser *p, EventDecl *ev) {
    if (!at(p, TK_IDENT)) {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                  "expected an event handler name");
        diag_hint(p->diag, "events include state_entry, state_exit, touch_start, on_rez, listen, timer, http_response, ...");
        return 0;
    }
    SrcLoc loc = p->cur.loc;
    ev->name = p->cur.v.s; p->cur.v.s = NULL;
    ev->loc = loc;
    advance(p);
    expect(p, TK_LPAREN, "event handler header is:  name(<params>) { ... }");
    parse_params(p, &ev->params, &ev->n_params);
    expect(p, TK_RPAREN, "expected ')' closing event handler parameters");
    if (!at(p, TK_LBRACE)) {
        diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                  "expected '{' to begin event handler body");
        return 0;
    }
    ev->body = parse_block(p);
    return 1;
}

static int parse_state(Parser *p, StateDecl *st) {
    SrcLoc loc = p->cur.loc;
    if (eat(p, TK_KW_DEFAULT)) {
        st->name = xstrdup("default");
        st->is_default = 1;
    } else if (eat(p, TK_KW_STATE)) {
        if (!at(p, TK_IDENT)) {
            diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                      "expected a state name after 'state'");
            diag_hint(p->diag, "named states are declared with:  state name { events... }");
            return 0;
        }
        st->name = p->cur.v.s; p->cur.v.s = NULL;
        st->is_default = 0;
        advance(p);
    } else {
        return 0;
    }
    st->loc = loc;
    if (!expect(p, TK_LBRACE, "state bodies are enclosed in '{ }'")) return 0;
    EventDecl *evs = NULL; int n = 0, cap = 0;
    while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
        if (n == cap) { cap = cap ? cap * 2 : 4; evs = (EventDecl*)xrealloc(evs, sizeof(*evs) * cap); memset(evs + n, 0, sizeof(*evs) * (cap - n)); }
        memset(&evs[n], 0, sizeof evs[n]);
        if (!parse_event(p, &evs[n])) {
            /* recover by scanning to next '}' at outer level */
            int depth = 0;
            while (!at(p, TK_EOF)) {
                if (at(p, TK_LBRACE)) depth++;
                else if (at(p, TK_RBRACE)) { if (depth == 0) break; depth--; }
                advance(p);
            }
            break;
        }
        n++;
    }
    expect(p, TK_RBRACE, "expected '}' closing the state");
    st->events = evs; st->n_events = n;
    return 1;
}

void parser_init(Parser *p, Lexer *lex, DiagCtx *diag) {
    memset(p, 0, sizeof *p);
    p->lex = lex; p->diag = diag;
    prime(p);
}

int parser_parse_script(Parser *p, ScriptAST *out) {
    memset(out, 0, sizeof *out);
    out->file = p->lex->file;

    int g_cap = 0, f_cap = 0, s_cap = 0;
    while (!at(p, TK_EOF)) {
        if (diag_should_stop(p->diag)) break;
        SrcLoc loc = p->cur.loc;

        if (at(p, TK_KW_DEFAULT) || at(p, TK_KW_STATE)) {
            if (out->n_states == s_cap) {
                s_cap = s_cap ? s_cap * 2 : 4;
                out->states = (StateDecl*)xrealloc(out->states, sizeof(*out->states) * s_cap);
            }
            memset(&out->states[out->n_states], 0, sizeof(StateDecl));
            if (parse_state(p, &out->states[out->n_states])) out->n_states++;
            continue;
        }

        if (is_type_kw(p->cur.kind)) {
            /* Could be a global variable or a function with return type. */
            TypeKind t = tk_to_type(p->cur.kind);
            SrcLoc tloc = p->cur.loc;
            advance(p);
            if (!at(p, TK_IDENT)) {
                diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                          "expected name after type '%s'", type_name(t));
                continue;
            }
            char *name = p->cur.v.s; p->cur.v.s = NULL;
            SrcLoc nloc = p->cur.loc;
            advance(p);
            if (at(p, TK_LPAREN)) {
                advance(p);
                Param *params = NULL; int np = 0;
                parse_params(p, &params, &np);
                expect(p, TK_RPAREN, "expected ')' closing function parameters");
                if (!at(p, TK_LBRACE)) {
                    diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                              "expected '{' to begin function body");
                    free(name);
                    continue;
                }
                Stmt *body = parse_block(p);
                if (out->n_funcs == f_cap) {
                    f_cap = f_cap ? f_cap * 2 : 4;
                    out->funcs = (FuncDecl*)xrealloc(out->funcs, sizeof(*out->funcs) * f_cap);
                }
                FuncDecl *fd = &out->funcs[out->n_funcs++];
                memset(fd, 0, sizeof *fd);
                fd->ret = t; fd->has_return_type = 1;
                fd->name = name;
                fd->params = params; fd->n_params = np;
                fd->body = body;
                fd->loc = tloc;
                continue;
            }
            /* global variable */
            Expr *init = NULL;
            if (eat(p, TK_ASSIGN)) init = parse_assign(p);
            expect(p, TK_SEMI, "global declarations end with ';'");
            if (out->n_globals == g_cap) {
                g_cap = g_cap ? g_cap * 2 : 8;
                out->globals = (GlobalVar*)xrealloc(out->globals, sizeof(*out->globals) * g_cap);
            }
            GlobalVar *gv = &out->globals[out->n_globals++];
            gv->type = t; gv->name = name; gv->init = init; gv->loc = nloc;
            continue;
        }

        if (at(p, TK_IDENT)) {
            /* Procedure (void function) without explicit return type */
            char *name = p->cur.v.s; p->cur.v.s = NULL;
            SrcLoc nloc = p->cur.loc;
            advance(p);
            if (!at(p, TK_LPAREN)) {
                diag_emit(p->diag, DIAG_ERROR, nloc,
                          "unexpected identifier '%s' at top level", name);
                diag_hint(p->diag,
                    "top-level forms are: global variables, function definitions, and state blocks. Did you mean to declare a global like  integer %s = 0;  or open a function with  %s(...) { ... } ?",
                    name, name);
                free(name);
                advance(p);
                continue;
            }
            advance(p);
            Param *params = NULL; int np = 0;
            parse_params(p, &params, &np);
            expect(p, TK_RPAREN, "expected ')' closing function parameters");
            if (!at(p, TK_LBRACE)) {
                diag_emit(p->diag, DIAG_ERROR, p->cur.loc,
                          "expected '{' to begin function body");
                free(name);
                continue;
            }
            Stmt *body = parse_block(p);
            if (out->n_funcs == f_cap) {
                f_cap = f_cap ? f_cap * 2 : 4;
                out->funcs = (FuncDecl*)xrealloc(out->funcs, sizeof(*out->funcs) * f_cap);
            }
            FuncDecl *fd = &out->funcs[out->n_funcs++];
            memset(fd, 0, sizeof *fd);
            fd->ret = T_VOID; fd->has_return_type = 0;
            fd->name = name;
            fd->params = params; fd->n_params = np;
            fd->body = body;
            fd->loc = nloc;
            continue;
        }

        diag_emit(p->diag, DIAG_ERROR, loc,
                  "expected a global declaration, function, or state block, found %s",
                  cur_lexeme(p));
        diag_hint(p->diag,
            "a top-level LSL file looks like:\n"
            "    integer counter = 0;\n"
            "    say_hello() { llSay(0, \"hi\"); }\n"
            "    default { state_entry() { ... } }");
        advance(p);
    }
    return p->diag->errors == 0;
}
