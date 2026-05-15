/* fold.c - compile-time constant folder.
 *
 * Mono's official LSL compiler folds global initializers ("integer x = 1+2;"
 * → 8) and rejects any non-foldable initializer (function calls, identifier
 * references to other globals or builtins). We mirror that behaviour so a
 * script that compiles here is the same one the SL uploader accepts.
 */
#include "lsl.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static Expr *make_int_lit(long long v, SrcLoc loc) {
    Expr *e = (Expr*)xcalloc(1, sizeof(Expr));
    e->kind = E_INT_LIT; e->loc = loc; e->end_off = loc.off + 1;
    e->u.ilit = v; e->type = T_INTEGER;
    return e;
}
static Expr *make_float_lit(double v, SrcLoc loc) {
    Expr *e = (Expr*)xcalloc(1, sizeof(Expr));
    e->kind = E_FLOAT_LIT; e->loc = loc; e->end_off = loc.off + 1;
    e->u.flit = v; e->type = T_FLOAT;
    return e;
}
static Expr *make_string_lit(const char *s, SrcLoc loc) {
    Expr *e = (Expr*)xcalloc(1, sizeof(Expr));
    e->kind = E_STRING_LIT; e->loc = loc; e->end_off = loc.off + 1;
    e->u.slit = xstrdup(s); e->type = T_STRING;
    return e;
}

static int expr_is_literal(Expr *e) {
    if (!e) return 0;
    return e->kind == E_INT_LIT || e->kind == E_FLOAT_LIT || e->kind == E_STRING_LIT;
}

int expr_is_constant(Expr *e) {
    if (!e) return 1;
    switch (e->kind) {
        case E_INT_LIT:
        case E_FLOAT_LIT:
        case E_STRING_LIT:
            return 1;
        case E_VECTOR_LIT:
            return expr_is_constant(e->u.vec.x)
                && expr_is_constant(e->u.vec.y)
                && expr_is_constant(e->u.vec.z);
        case E_ROT_LIT:
            return expr_is_constant(e->u.vec.x)
                && expr_is_constant(e->u.vec.y)
                && expr_is_constant(e->u.vec.z)
                && expr_is_constant(e->u.vec.w);
        case E_LIST_LIT:
            for (int i = 0; i < e->u.list.n; i++)
                if (!expr_is_constant(e->u.list.items[i])) return 0;
            return 1;
        case E_IDENT: {
            /* Only built-in constants (TRUE, PI, …) qualify. */
            const BuiltinConst *bc = bi_lookup_const(e->u.ident);
            return bc != NULL;
        }
        case E_CAST: return expr_is_constant(e->u.cast.inner);
        case E_UNARY: return expr_is_constant(e->u.un.inner);
        case E_POSTFIX: return 0;
        case E_BINARY:
            return expr_is_constant(e->u.bin.l) && expr_is_constant(e->u.bin.r);
        case E_ASSIGN: return 0;
        case E_MEMBER: return expr_is_constant(e->u.memb.base);
        case E_CALL: return 0;
    }
    return 0;
}

/* Try to get the integer/float/string value of a constant expr.
 * Returns 0 on failure. */
static int as_int(Expr *e, long long *out) {
    if (!e) return 0;
    if (e->kind == E_INT_LIT) { *out = e->u.ilit; return 1; }
    if (e->kind == E_FLOAT_LIT) { *out = (long long)e->u.flit; return 1; }
    if (e->kind == E_IDENT) {
        const BuiltinConst *bc = bi_lookup_const(e->u.ident);
        if (bc && bc->type == T_INTEGER) { *out = bc->ival; return 1; }
    }
    return 0;
}
static int as_float(Expr *e, double *out) {
    if (!e) return 0;
    if (e->kind == E_INT_LIT) { *out = (double)e->u.ilit; return 1; }
    if (e->kind == E_FLOAT_LIT) { *out = e->u.flit; return 1; }
    if (e->kind == E_IDENT) {
        const BuiltinConst *bc = bi_lookup_const(e->u.ident);
        if (bc && bc->type == T_FLOAT)   { *out = bc->fval; return 1; }
        if (bc && bc->type == T_INTEGER) { *out = (double)bc->ival; return 1; }
    }
    return 0;
}
static int as_string(Expr *e, const char **out) {
    if (!e) return 0;
    if (e->kind == E_STRING_LIT) { *out = e->u.slit ? e->u.slit : ""; return 1; }
    if (e->kind == E_IDENT) {
        const BuiltinConst *bc = bi_lookup_const(e->u.ident);
        if (bc && (bc->type == T_STRING || bc->type == T_KEY)) {
            *out = bc->sval ? bc->sval : "";
            return 1;
        }
    }
    return 0;
}

static char *concat(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = (char*)xmalloc(la + lb + 1);
    memcpy(r, a, la); memcpy(r + la, b, lb); r[la + lb] = '\0';
    return r;
}

/* Fold a binary numeric expression. Returns a new literal, or NULL if it
 * cannot be folded. */
static Expr *fold_binary(int op, Expr *l, Expr *r, SrcLoc loc) {
    /* String concat: "a" + "b" */
    if (op == TK_PLUS) {
        const char *sa = NULL, *sb = NULL;
        if (as_string(l, &sa) && as_string(r, &sb)) {
            char *c = concat(sa, sb);
            Expr *e = make_string_lit(c, loc);
            free(c);
            return e;
        }
    }
    /* Pure integer arithmetic. */
    long long ai, bi;
    if (as_int(l, &ai) && as_int(r, &bi) &&
        l->type != T_FLOAT && r->type != T_FLOAT &&
        l->kind != E_FLOAT_LIT && r->kind != E_FLOAT_LIT)
    {
        switch (op) {
            case TK_PLUS:    return make_int_lit(ai + bi, loc);
            case TK_MINUS:   return make_int_lit(ai - bi, loc);
            case TK_STAR:    return make_int_lit(ai * bi, loc);
            case TK_SLASH:   if (bi == 0) return NULL; return make_int_lit(ai / bi, loc);
            case TK_PERCENT: if (bi == 0) return NULL; return make_int_lit(ai % bi, loc);
            case TK_AND:     return make_int_lit(ai & bi, loc);
            case TK_OR:      return make_int_lit(ai | bi, loc);
            case TK_XOR:     return make_int_lit(ai ^ bi, loc);
            case TK_SHL:     return make_int_lit((long long)((unsigned long long)ai << (bi & 31)), loc);
            case TK_SHR:     return make_int_lit(ai >> (bi & 31), loc);
            case TK_EQ:      return make_int_lit(ai == bi, loc);
            case TK_NEQ:     return make_int_lit(ai != bi, loc);
            case TK_LT:      return make_int_lit(ai <  bi, loc);
            case TK_GT:      return make_int_lit(ai >  bi, loc);
            case TK_LE:      return make_int_lit(ai <= bi, loc);
            case TK_GE:      return make_int_lit(ai >= bi, loc);
            case TK_LAND:    return make_int_lit(ai && bi, loc);
            case TK_LOR:     return make_int_lit(ai || bi, loc);
            default: break;
        }
    }
    /* Float arithmetic (any operand float). */
    double ad, bd;
    if (as_float(l, &ad) && as_float(r, &bd)) {
        switch (op) {
            case TK_PLUS:  return make_float_lit(ad + bd, loc);
            case TK_MINUS: return make_float_lit(ad - bd, loc);
            case TK_STAR:  return make_float_lit(ad * bd, loc);
            case TK_SLASH: if (bd == 0.0) return NULL; return make_float_lit(ad / bd, loc);
            case TK_EQ:    return make_int_lit(ad == bd, loc);
            case TK_NEQ:   return make_int_lit(ad != bd, loc);
            case TK_LT:    return make_int_lit(ad <  bd, loc);
            case TK_GT:    return make_int_lit(ad >  bd, loc);
            case TK_LE:    return make_int_lit(ad <= bd, loc);
            case TK_GE:    return make_int_lit(ad >= bd, loc);
            case TK_LAND:  return make_int_lit((ad != 0.0) && (bd != 0.0), loc);
            case TK_LOR:   return make_int_lit((ad != 0.0) || (bd != 0.0), loc);
            default: break;
        }
    }
    return NULL;
}

static Expr *fold_unary(int op, Expr *inner, SrcLoc loc) {
    long long ai;
    if (as_int(inner, &ai) && inner->type != T_FLOAT && inner->kind != E_FLOAT_LIT) {
        switch (op) {
            case TK_MINUS: return make_int_lit(-ai, loc);
            case TK_PLUS:  return make_int_lit( ai, loc);
            case TK_TILDE: return make_int_lit(~ai, loc);
            case TK_NOT:   return make_int_lit(!ai, loc);
            default: break;
        }
    }
    double ad;
    if (as_float(inner, &ad)) {
        switch (op) {
            case TK_MINUS: return make_float_lit(-ad, loc);
            case TK_PLUS:  return make_float_lit( ad, loc);
            case TK_NOT:   return make_int_lit(ad == 0.0, loc);
            default: break;
        }
    }
    return NULL;
}

/* Fold (T) literal — handle the casts that can be evaluated at compile
 * time. We only handle ones with well-defined semantics. */
static Expr *fold_cast(TypeKind to, Expr *inner, SrcLoc loc) {
    if (to == T_STRING) {
        char buf[64]; const char *s;
        long long ai; double ad;
        if (as_string(inner, &s)) return make_string_lit(s, loc);
        if (as_int(inner, &ai) && inner->type != T_FLOAT && inner->kind != E_FLOAT_LIT) {
            snprintf(buf, sizeof buf, "%lld", ai);
            return make_string_lit(buf, loc);
        }
        if (as_float(inner, &ad)) {
            snprintf(buf, sizeof buf, "%.6f", ad);
            return make_string_lit(buf, loc);
        }
        return NULL;
    }
    if (to == T_INTEGER) {
        long long ai; double ad; const char *s;
        if (as_int(inner, &ai)) return make_int_lit(ai, loc);
        if (as_float(inner, &ad)) return make_int_lit((long long)ad, loc);
        if (as_string(inner, &s)) return make_int_lit(strtoll(s, NULL, 10), loc);
        return NULL;
    }
    if (to == T_FLOAT) {
        long long ai; double ad; const char *s;
        if (as_float(inner, &ad)) return make_float_lit(ad, loc);
        if (as_int(inner, &ai))   return make_float_lit((double)ai, loc);
        if (as_string(inner, &s)) return make_float_lit(strtod(s, NULL), loc);
        return NULL;
    }
    if (to == T_KEY) {
        const char *s;
        if (as_string(inner, &s)) {
            Expr *e = make_string_lit(s, loc);
            e->type = T_KEY;
            return e;
        }
    }
    return NULL;
}

int fold_expr(Expr **pe, DiagCtx *d) {
    (void)d;
    Expr *e = *pe;
    if (!e) return 1;
    switch (e->kind) {
        case E_INT_LIT: case E_FLOAT_LIT: case E_STRING_LIT:
            return 1;
        case E_IDENT: {
            const BuiltinConst *bc = bi_lookup_const(e->u.ident);
            if (!bc) return 0;
            Expr *r = NULL;
            switch (bc->type) {
                case T_INTEGER: r = make_int_lit(bc->ival, e->loc); break;
                case T_FLOAT:   r = make_float_lit(bc->fval, e->loc); break;
                case T_STRING:
                case T_KEY:     r = make_string_lit(bc->sval ? bc->sval : "", e->loc);
                                r->type = bc->type;
                                break;
                default: return 1;  /* leave as-is */
            }
            if (r) { ast_free_expr(*pe); *pe = r; }
            return 1;
        }
        case E_VECTOR_LIT:
            fold_expr(&e->u.vec.x, d);
            fold_expr(&e->u.vec.y, d);
            fold_expr(&e->u.vec.z, d);
            return expr_is_literal(e->u.vec.x) && expr_is_literal(e->u.vec.y) && expr_is_literal(e->u.vec.z);
        case E_ROT_LIT:
            fold_expr(&e->u.vec.x, d);
            fold_expr(&e->u.vec.y, d);
            fold_expr(&e->u.vec.z, d);
            fold_expr(&e->u.vec.w, d);
            return 1;
        case E_LIST_LIT:
            for (int i = 0; i < e->u.list.n; i++) fold_expr(&e->u.list.items[i], d);
            return 1;
        case E_UNARY: {
            fold_expr(&e->u.un.inner, d);
            Expr *r = fold_unary(e->u.un.op, e->u.un.inner, e->loc);
            if (r) { ast_free_expr(*pe); *pe = r; return 1; }
            return 0;
        }
        case E_BINARY: {
            fold_expr(&e->u.bin.l, d);
            fold_expr(&e->u.bin.r, d);
            Expr *r = fold_binary(e->u.bin.op, e->u.bin.l, e->u.bin.r, e->loc);
            if (r) { ast_free_expr(*pe); *pe = r; return 1; }
            return 0;
        }
        case E_CAST: {
            fold_expr(&e->u.cast.inner, d);
            Expr *r = fold_cast(e->u.cast.to, e->u.cast.inner, e->loc);
            if (r) { ast_free_expr(*pe); *pe = r; return 1; }
            return 0;
        }
        case E_MEMBER: {
            /* member-of-vector-literal can fold: <1,2,3>.y -> 2.0 */
            fold_expr(&e->u.memb.base, d);
            Expr *b = e->u.memb.base;
            if (!b) return 0;
            if (b->kind == E_VECTOR_LIT || b->kind == E_ROT_LIT) {
                Expr *p = NULL;
                switch (e->u.memb.member) {
                    case 'x': p = b->u.vec.x; break;
                    case 'y': p = b->u.vec.y; break;
                    case 'z': p = b->u.vec.z; break;
                    case 's': p = b->u.vec.w; break;
                }
                if (p && expr_is_literal(p)) {
                    Expr *r = NULL;
                    if (p->kind == E_FLOAT_LIT) r = make_float_lit(p->u.flit, e->loc);
                    else if (p->kind == E_INT_LIT) r = make_float_lit((double)p->u.ilit, e->loc);
                    if (r) { ast_free_expr(*pe); *pe = r; return 1; }
                }
            }
            return 0;
        }
        case E_POSTFIX:
        case E_ASSIGN:
        case E_CALL:
            return 0;
    }
    return 0;
}
