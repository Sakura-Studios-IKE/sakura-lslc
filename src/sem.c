/* sem.c - LSL semantic analysis (name resolution + typechecking) */
#include "lsl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External arity-aware lookup (defined in builtins.c) */
const BuiltinFn *bi_lookup_fn_arity(const char *name, int n_args);

typedef struct LabelList {
    char *name;
    SrcLoc loc;
    struct LabelList *next;
} LabelList;

typedef struct {
    DiagCtx *diag;
    SymTab st;
    ScriptAST *ast;
    FuncDecl *cur_func;
    EventDecl *cur_event;
    StateDecl *cur_state;
    int in_event;        /* inside an event => state-change allowed */
    /* per-function label/jump tracking */
    LabelList *labels;
    LabelList *jumps;    /* unresolved jumps */
} Sem;

/* ----------------------------- Forward decls ---------------------------- */
static void check_stmt(Sem *s, Stmt *st);
static void check_expr(Sem *s, Expr *e);
static const char *expr_lvalue_kind(Expr *e);

/* ----------------------- Suggestion helpers ----------------------------- */

/* Build candidate list from all known idents currently in scope plus builtins,
 * and return a "did you mean?" if one exists. The result is allocated in a
 * static buffer (formatted), so callers should consume it immediately.
 *
 * Strategy: collect all candidate names into a temporary array, then call
 * diag_did_you_mean.
 */

static int collect_scope_names(Sem *s, const char ***out_arr) {
    /* Walk scopes; gather names */
    int cap = 64, n = 0;
    const char **arr = (const char**)xmalloc(sizeof(char*) * cap);
    Scope *sc = s->st.cur;
    while (sc) {
        Sym *cur = sc->list;
        while (cur) {
            if (n == cap) { cap *= 2; arr = (const char**)xrealloc(arr, sizeof(char*) * cap); }
            arr[n++] = cur->name;
            cur = cur->next_in_scope;
        }
        sc = sc->parent;
    }
    *out_arr = arr;
    return n;
}

static void suggest_ident(Sem *s, const char *name, SrcLoc loc) {
    (void)loc;
    const char **names; int n = collect_scope_names(s, &names);
    /* Append constants */
    int extra = BI_CONST_N + BI_FN_N;
    names = (const char**)xrealloc(names, sizeof(char*) * (n + extra));
    for (int i = 0; i < BI_CONST_N; i++) names[n++] = BI_CONST[i].name;
    for (int i = 0; i < BI_FN_N; i++) names[n++] = BI_FN[i].name;
    const char *guess = diag_did_you_mean(name, names, n, 2);
    free(names);
    if (guess) diag_hint(s->diag, "did you mean '%s'?", guess);
}

static void suggest_fn(Sem *s, const char *name, int n_args) {
    (void)n_args;
    const char **names; int n = collect_scope_names(s, &names);
    names = (const char**)xrealloc(names, sizeof(char*) * (n + BI_FN_N));
    for (int i = 0; i < BI_FN_N; i++) names[n++] = BI_FN[i].name;
    const char *guess = diag_did_you_mean(name, names, n, 2);
    free(names);
    if (guess) diag_hint(s->diag, "did you mean '%s'?", guess);
}

static void suggest_event(Sem *s, const char *name) {
    (void)s;
    const char **names = (const char**)xmalloc(sizeof(char*) * BI_EVENT_N);
    for (int i = 0; i < BI_EVENT_N; i++) names[i] = BI_EVENT[i].name;
    const char *guess = diag_did_you_mean(name, names, BI_EVENT_N, 2);
    free(names);
    if (guess) diag_hint(s->diag, "did you mean event '%s'?", guess);
}

static void suggest_state(Sem *s, const char *name) {
    int n = s->ast->n_states;
    const char **names = (const char**)xmalloc(sizeof(char*) * n);
    for (int i = 0; i < n; i++) names[i] = s->ast->states[i].name;
    const char *guess = diag_did_you_mean(name, names, n, 2);
    free(names);
    if (guess) diag_hint(s->diag, "did you mean state '%s'?", guess);
    else if (n > 0) {
        diag_hint(s->diag, "defined states are:");
        for (int i = 0; i < n; i++) diag_hint(s->diag, "  - %s", s->ast->states[i].name);
    }
}

/* ----------------------------- Type checking ---------------------------- */

static int is_lvalue(Expr *e) {
    if (!e) return 0;
    if (e->kind == E_IDENT) return e->is_lvalue;
    if (e->kind == E_MEMBER) return e->u.memb.base && is_lvalue(e->u.memb.base);
    return 0;
}

static TypeKind member_type(char m, TypeKind base) {
    if (base == T_VECTOR) {
        if (m == 'x' || m == 'y' || m == 'z') return T_FLOAT;
        return T_ERROR;
    }
    if (base == T_ROTATION) {
        if (m == 'x' || m == 'y' || m == 'z' || m == 's') return T_FLOAT;
        return T_ERROR;
    }
    return T_ERROR;
}

static const char *expr_lvalue_kind(Expr *e) {
    switch (e->kind) {
        case E_INT_LIT: case E_FLOAT_LIT: case E_STRING_LIT:
            return "a literal";
        case E_CAST: return "a cast expression";
        case E_CALL: return "a function call";
        case E_BINARY: return "a binary expression";
        case E_UNARY: return "a unary expression";
        case E_POSTFIX: return "a postfix expression";
        case E_VECTOR_LIT: case E_ROT_LIT: return "a vector/rotation literal";
        case E_LIST_LIT: return "a list literal";
        default: return "this expression";
    }
}

static void mark_used(Sym *s) { if (s) s->used = 1; }

static int operator_lexeme_idx(int op) { (void)op; return 0; }

static const char *op_name(int op) {
    switch (op) {
        case TK_PLUS: return "+";
        case TK_MINUS: return "-";
        case TK_STAR: return "*";
        case TK_SLASH: return "/";
        case TK_PERCENT: return "%";
        case TK_AND: return "&";
        case TK_OR: return "|";
        case TK_XOR: return "^";
        case TK_SHL: return "<<";
        case TK_SHR: return ">>";
        case TK_LAND: return "&&";
        case TK_LOR: return "||";
        case TK_EQ: return "==";
        case TK_NEQ: return "!=";
        case TK_LT: return "<";
        case TK_GT: return ">";
        case TK_LE: return "<=";
        case TK_GE: return ">=";
        case TK_ASSIGN: return "=";
        case TK_PLUS_ASSIGN: return "+=";
        case TK_MINUS_ASSIGN: return "-=";
        case TK_STAR_ASSIGN: return "*=";
        case TK_SLASH_ASSIGN: return "/=";
        case TK_PERCENT_ASSIGN: return "%=";
        case TK_INC: return "++";
        case TK_DEC: return "--";
        case TK_NOT: return "!";
        case TK_TILDE: return "~";
    }
    return "?";
}

static int compatible_compare(TypeKind a, TypeKind b) {
    if (a == T_ERROR || b == T_ERROR) return 1;
    if (a == b) return 1;
    if (type_is_numeric(a) && type_is_numeric(b)) return 1;
    if ((a == T_KEY && b == T_STRING) || (a == T_STRING && b == T_KEY)) return 1;
    return 0;
}

/* Implicit conversions in argument-passing context. Same rules as
 * assignment, except that we ALSO accept passing typed literals in
 * list contexts. Returns 1 if OK. */
static int param_assignable(TypeKind want, TypeKind got) {
    if (want == T_ANY || got == T_ANY) return 1;
    return type_implicit_assignable(want, got);
}

static const char *un_op_name(int op) {
    switch (op) {
        case TK_MINUS: return "unary -";
        case TK_PLUS:  return "unary +";
        case TK_NOT:   return "!";
        case TK_TILDE: return "~";
        case TK_INC:   return "++";
        case TK_DEC:   return "--";
    }
    return "?";
}

/* Type check a call expression. Sets e->type. */
static void check_call(Sem *s, Expr *e) {
    /* Look up user function or built-in */
    const char *name = e->u.call.name;
    int nargs = e->u.call.n_args;
    for (int i = 0; i < nargs; i++) check_expr(s, e->u.call.args[i]);

    Sym *us = sym_lookup_global(&s->st, name);
    if (us && us->kind == SYM_FUNC) {
        mark_used(us);
        if (us->n_params != nargs) {
            diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                "call to '%s' has %d argument%s but %d %s expected",
                name, nargs, nargs == 1 ? "" : "s",
                us->n_params, us->n_params == 1 ? "is" : "are");
            diag_note(s->diag, us->loc, "'%s' is declared here", name);
        } else {
            for (int i = 0; i < nargs; i++) {
                TypeKind got = e->u.call.args[i]->type;
                TypeKind want = us->params[i];
                if (!param_assignable(want, got)) {
                    diag_emit_range(s->diag, DIAG_ERROR,
                        e->u.call.args[i]->loc, e->u.call.args[i]->end_off,
                        "argument %d of '%s' has type %s, but the parameter is declared as %s",
                        i + 1, name, type_name(got), type_name(want));
                    diag_hint(s->diag,
                        "wrap the argument in a cast like (%s) to convert explicitly",
                        type_name(want));
                    diag_note(s->diag, us->loc, "'%s' is declared here", name);
                }
            }
        }
        e->type = us->type;
        return;
    }

    const BuiltinFn *bi = bi_lookup_fn_arity(name, nargs);
    if (!bi) bi = bi_lookup_fn(name);
    if (bi) {
        if ((bi->flags & BFN_MONO_ONLY) && s->diag->target == LSL_LSO) {
            diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                "'%s' is a Mono-only built-in and is not available under LSO",
                name);
            diag_hint(s->diag,
                "remove the --lso flag to target Mono, or use an LSO-compatible alternative");
        }
        if (bi->n_params != nargs) {
            diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                "built-in '%s' expects %d argument%s but %d %s given",
                name, bi->n_params, bi->n_params == 1 ? "" : "s",
                nargs, nargs == 1 ? "was" : "were");
            diag_hint(s->diag, "check the LSL Wiki entry for %s for the correct signature", name);
            e->type = bi->ret;
            return;
        }
        for (int i = 0; i < nargs; i++) {
            TypeKind got = e->u.call.args[i]->type;
            TypeKind want = bi->params[i];
            if (want == T_VOID || want == T_ANY) continue;
            if (!param_assignable(want, got)) {
                diag_emit_range(s->diag, DIAG_ERROR,
                    e->u.call.args[i]->loc, e->u.call.args[i]->end_off,
                    "argument %d of built-in '%s' has type %s, but it expects %s",
                    i + 1, name, type_name(got), type_name(want));
                diag_hint(s->diag, "if the conversion is intended, write it explicitly: (%s)expr", type_name(want));
            }
        }
        e->type = bi->ret;
        return;
    }

    /* Unknown function. If the prefix is "ll" we warn (likely a new LSL fn),
     * otherwise it's an error. */
    int looks_lsl = name && name[0] == 'l' && name[1] == 'l';
    if (looks_lsl) {
        diag_emit_range(s->diag,
            DIAG_WARNING, e->loc, e->end_off,
            "call to unknown ll-prefixed function '%s' — assuming it exists with the types you passed", name);
        diag_hint(s->diag, "if this is a typo, see the suggestion below; otherwise this LSL function may be newer than our built-in table");
        suggest_fn(s, name, nargs);
        e->type = T_ANY;
    } else {
        diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
            "call to undeclared function '%s'", name);
        suggest_fn(s, name, nargs);
        e->type = T_ERROR;
    }
}

static void check_expr(Sem *s, Expr *e) {
    if (!e) return;
    switch (e->kind) {
        case E_INT_LIT:   e->type = T_INTEGER; return;
        case E_FLOAT_LIT: e->type = T_FLOAT;   return;
        case E_STRING_LIT:e->type = T_STRING;  return;
        case E_VECTOR_LIT:
            check_expr(s, e->u.vec.x);
            check_expr(s, e->u.vec.y);
            check_expr(s, e->u.vec.z);
            /* Each component must be convertible to float. */
            {
                Expr *cs[3] = {e->u.vec.x, e->u.vec.y, e->u.vec.z};
                const char *axes[3] = {"x", "y", "z"};
                for (int i = 0; i < 3; i++) {
                    if (cs[i]->type != T_FLOAT && cs[i]->type != T_INTEGER &&
                        cs[i]->type != T_ERROR) {
                        diag_emit_range(s->diag, DIAG_ERROR, cs[i]->loc, cs[i]->end_off,
                            "vector component %s has type %s; expected float (or integer)",
                            axes[i], type_name(cs[i]->type));
                    }
                }
            }
            e->type = T_VECTOR;
            return;
        case E_ROT_LIT:
            check_expr(s, e->u.vec.x);
            check_expr(s, e->u.vec.y);
            check_expr(s, e->u.vec.z);
            check_expr(s, e->u.vec.w);
            e->type = T_ROTATION;
            return;
        case E_LIST_LIT:
            for (int i = 0; i < e->u.list.n; i++) check_expr(s, e->u.list.items[i]);
            e->type = T_LIST;
            return;
        case E_IDENT: {
            Sym *sym = sym_lookup(&s->st, e->u.ident);
            if (!sym) {
                const BuiltinConst *bc = bi_lookup_const(e->u.ident);
                if (bc) { e->type = bc->type; e->is_lvalue = 0; return; }
                /* Sometimes people misuse type names as identifiers */
                if (type_from_keyword(e->u.ident) != T_ERROR) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "'%s' is a type, not an expression", e->u.ident);
                    diag_hint(s->diag, "did you forget to write a variable name after the type? e.g.  %s name", e->u.ident);
                    e->type = T_ERROR;
                    return;
                }
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "undeclared identifier '%s'", e->u.ident);
                suggest_ident(s, e->u.ident, e->loc);
                e->type = T_ERROR;
                return;
            }
            mark_used(sym);
            if (sym->kind == SYM_FUNC) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "function '%s' used as a value", e->u.ident);
                diag_hint(s->diag, "call the function with parentheses: %s(...)", e->u.ident);
                e->type = T_ERROR; return;
            }
            if (sym->kind == SYM_STATE) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "state '%s' used as a value", e->u.ident);
                diag_hint(s->diag, "states are not expressions — to switch state, write:  state %s;", e->u.ident);
                e->type = T_ERROR; return;
            }
            e->type = sym->type;
            e->is_lvalue = (sym->kind == SYM_GLOBAL || sym->kind == SYM_LOCAL || sym->kind == SYM_PARAM);
            return;
        }
        case E_MEMBER: {
            check_expr(s, e->u.memb.base);
            TypeKind bt = e->u.memb.base->type;
            char m = e->u.memb.member;
            if (bt == T_ERROR) { e->type = T_ERROR; return; }
            if (bt != T_VECTOR && bt != T_ROTATION) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "member access '.%c' applied to %s; only vector and rotation have members",
                    m ? m : '?', type_name(bt));
                e->type = T_ERROR;
                return;
            }
            TypeKind mt = member_type(m, bt);
            if (mt == T_ERROR) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "%s has no member '%c'", type_name(bt), m);
                if (bt == T_VECTOR)
                    diag_hint(s->diag, "vectors have members .x .y .z");
                else
                    diag_hint(s->diag, "rotations have members .x .y .z .s");
                e->type = T_ERROR;
                return;
            }
            e->type = mt;
            e->is_lvalue = is_lvalue(e->u.memb.base);
            return;
        }
        case E_CAST: {
            check_expr(s, e->u.cast.inner);
            TypeKind from = e->u.cast.inner->type;
            TypeKind to = e->u.cast.to;
            if (!type_explicit_castable(to, from)) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "cannot cast %s to %s", type_name(from), type_name(to));
                if (from == T_LIST && to != T_STRING && to != T_LIST) {
                    diag_hint(s->diag, "lists can only be cast to (string) — use llList2%s to extract a typed value",
                        to == T_INTEGER ? "Integer" : to == T_FLOAT ? "Float" : "String");
                }
            }
            e->type = to;
            return;
        }
        case E_UNARY: {
            check_expr(s, e->u.un.inner);
            TypeKind t = e->u.un.inner->type;
            int op = e->u.un.op;
            if (op == TK_NOT) {
                if (!(t == T_INTEGER || t == T_FLOAT || t == T_ERROR)) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "operator '!' requires a number, got %s", type_name(t));
                }
                e->type = T_INTEGER;
                return;
            }
            if (op == TK_TILDE) {
                if (t != T_INTEGER && t != T_ERROR) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "operator '~' requires an integer, got %s", type_name(t));
                }
                e->type = T_INTEGER;
                return;
            }
            if (op == TK_MINUS || op == TK_PLUS) {
                if (t == T_INTEGER || t == T_FLOAT || t == T_VECTOR || t == T_ROTATION || t == T_ERROR) {
                    e->type = t; return;
                }
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "operator '%s' is not defined for %s", un_op_name(op), type_name(t));
                e->type = T_ERROR; return;
            }
            if (op == TK_INC || op == TK_DEC) {
                if (!is_lvalue(e->u.un.inner)) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "operand of '%s' must be assignable; got %s", un_op_name(op),
                        expr_lvalue_kind(e->u.un.inner));
                }
                if (t != T_INTEGER && t != T_FLOAT && t != T_ERROR) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "operator '%s' is not defined for %s", un_op_name(op), type_name(t));
                }
                e->type = t; return;
            }
            e->type = T_ERROR; return;
        }
        case E_POSTFIX: {
            check_expr(s, e->u.post.inner);
            if (!is_lvalue(e->u.post.inner)) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "operand of '%s' must be assignable; got %s",
                    op_name(e->u.post.op), expr_lvalue_kind(e->u.post.inner));
            }
            TypeKind t = e->u.post.inner->type;
            if (t != T_INTEGER && t != T_FLOAT && t != T_ERROR) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "postfix '%s' is not defined for %s", op_name(e->u.post.op), type_name(t));
            }
            e->type = t;
            return;
        }
        case E_BINARY: {
            check_expr(s, e->u.bin.l);
            check_expr(s, e->u.bin.r);
            TypeKind lt = e->u.bin.l->type, rt = e->u.bin.r->type;
            int op = e->u.bin.op;
            /* Logical ops: any type accepted, returns integer */
            if (op == TK_LAND || op == TK_LOR) { e->type = T_INTEGER; return; }
            if (op == TK_EQ || op == TK_NEQ || op == TK_LT || op == TK_GT
                || op == TK_LE || op == TK_GE) {
                if (!compatible_compare(lt, rt)) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "comparison '%s' between incompatible types %s and %s",
                        op_name(op), type_name(lt), type_name(rt));
                    if ((lt == T_LIST) ^ (rt == T_LIST))
                        diag_hint(s->diag, "lists compare only against other lists (and equality compares length, not contents)");
                }
                e->type = T_INTEGER;
                return;
            }
            TypeKind r = type_binop_result(op, lt, rt);
            if (r == T_ERROR) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "operator '%s' is not defined for operands of type %s and %s",
                    op_name(op), type_name(lt), type_name(rt));
                if (op == TK_PLUS && (lt == T_STRING || rt == T_STRING) &&
                    type_is_numeric(lt == T_STRING ? rt : lt))
                    diag_hint(s->diag, "to build a string, cast the number explicitly: (string)num");
            }
            e->type = r;
            return;
        }
        case E_ASSIGN: {
            check_expr(s, e->u.asn.l);
            check_expr(s, e->u.asn.r);
            if (!is_lvalue(e->u.asn.l)) {
                diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                    "left-hand side of '%s' is %s, not an assignable target",
                    op_name(e->u.asn.op), expr_lvalue_kind(e->u.asn.l));
            }
            TypeKind lt = e->u.asn.l->type;
            TypeKind rt = e->u.asn.r->type;
            int op = e->u.asn.op;
            if (op != TK_ASSIGN) {
                /* Compound: simulate l op r, must yield assignable to lt. */
                int real_op = TK_PLUS;
                switch (op) {
                    case TK_PLUS_ASSIGN: real_op = TK_PLUS; break;
                    case TK_MINUS_ASSIGN: real_op = TK_MINUS; break;
                    case TK_STAR_ASSIGN:  real_op = TK_STAR;  break;
                    case TK_SLASH_ASSIGN: real_op = TK_SLASH; break;
                    case TK_PERCENT_ASSIGN: real_op = TK_PERCENT; break;
                }
                TypeKind r = type_binop_result(real_op, lt, rt);
                if (r == T_ERROR) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "no operator '%s' defined for %s and %s",
                        op_name(op), type_name(lt), type_name(rt));
                } else if (!type_implicit_assignable(lt, r)) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "compound assignment '%s' would yield %s but left-hand side is %s",
                        op_name(op), type_name(r), type_name(lt));
                }
            } else {
                if (!type_implicit_assignable(lt, rt)) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->end_off,
                        "cannot assign %s to %s", type_name(rt), type_name(lt));
                    if (type_explicit_castable(lt, rt))
                        diag_hint(s->diag, "an explicit cast is required:  (%s)expr", type_name(lt));
                    else
                        diag_hint(s->diag, "the right-hand side type is not compatible with '%s'", type_name(lt));
                }
            }
            e->type = lt;
            return;
        }
        case E_CALL: check_call(s, e); return;
    }
}

/* ----------------------------- Statements ------------------------------- */

static void check_block(Sem *s, Stmt *b) {
    sym_push_scope(&s->st);
    for (int i = 0; i < b->u.block.n; i++) check_stmt(s, b->u.block.stmts[i]);
    sym_pop_scope(&s->st);
}

static void check_stmt(Sem *s, Stmt *st) {
    if (!st) return;
    switch (st->kind) {
        case S_EMPTY: return;
        case S_EXPR: check_expr(s, st->u.expr); return;
        case S_BLOCK: check_block(s, st); return;
        case S_IF:
            check_expr(s, st->u.if_s.cond);
            if (st->u.if_s.cond) {
                TypeKind t = st->u.if_s.cond->type;
                if (!(t == T_INTEGER || t == T_FLOAT || t == T_KEY ||
                      t == T_STRING || t == T_VECTOR || t == T_ROTATION ||
                      t == T_LIST || t == T_ERROR))
                {
                    diag_emit_range(s->diag, DIAG_ERROR, st->u.if_s.cond->loc, st->u.if_s.cond->end_off,
                        "'if' condition has type %s which is not testable", type_name(t));
                }
            }
            check_stmt(s, st->u.if_s.then_s);
            check_stmt(s, st->u.if_s.else_s);
            return;
        case S_WHILE:
            check_expr(s, st->u.while_s.cond);
            check_stmt(s, st->u.while_s.body);
            return;
        case S_DO:
            check_stmt(s, st->u.do_s.body);
            check_expr(s, st->u.do_s.cond);
            return;
        case S_FOR: {
            for (int i = 0; i < st->u.for_s.n_init; i++) check_expr(s, st->u.for_s.init[i]);
            check_expr(s, st->u.for_s.cond);
            for (int i = 0; i < st->u.for_s.n_post; i++) check_expr(s, st->u.for_s.post[i]);
            check_stmt(s, st->u.for_s.body);
            return;
        }
        case S_DECL: {
            Sym *existing = sym_lookup_local(&s->st, st->u.decl.name);
            if (existing) {
                diag_emit_range(s->diag, DIAG_ERROR, st->u.decl.name_loc, st->u.decl.name_loc.off + 1,
                    "redeclaration of '%s'", st->u.decl.name);
                diag_note(s->diag, existing->loc, "previously declared here");
            } else {
                /* Shadowing a global: emit a warning under -Wall */
                Sym *g = sym_lookup(&s->st, st->u.decl.name);
                if (g && (g->kind == SYM_GLOBAL || g->kind == SYM_PARAM) && s->diag->wall) {
                    diag_emit_range(s->diag, DIAG_WARNING, st->u.decl.name_loc, st->u.decl.name_loc.off + 1,
                        "local '%s' shadows %s of the same name",
                        st->u.decl.name, g->kind == SYM_GLOBAL ? "a global" : "a parameter");
                    diag_note(s->diag, g->loc, "shadowed declaration is here");
                }
                if (bi_lookup_const(st->u.decl.name)) {
                    diag_emit_range(s->diag, DIAG_WARNING, st->u.decl.name_loc, st->u.decl.name_loc.off + 1,
                        "local '%s' shadows built-in constant of the same name", st->u.decl.name);
                }
                sym_define(&s->st, st->u.decl.name, SYM_LOCAL, st->u.decl.t, st->u.decl.name_loc);
            }
            if (st->u.decl.init) {
                check_expr(s, st->u.decl.init);
                if (!type_implicit_assignable(st->u.decl.t, st->u.decl.init->type)) {
                    diag_emit_range(s->diag, DIAG_ERROR, st->u.decl.init->loc, st->u.decl.init->end_off,
                        "cannot initialize %s with %s",
                        type_name(st->u.decl.t), type_name(st->u.decl.init->type));
                    if (type_explicit_castable(st->u.decl.t, st->u.decl.init->type))
                        diag_hint(s->diag, "an explicit cast is required:  (%s)expr", type_name(st->u.decl.t));
                }
            }
            return;
        }
        case S_RETURN: {
            FuncDecl *f = s->cur_func;
            if (!f) {
                /* return in event handler — ok with no value, error with value */
                if (st->u.ret.has_value) {
                    diag_emit_range(s->diag, DIAG_ERROR, st->loc, st->loc.off + 6,
                        "event handlers cannot return a value");
                    diag_hint(s->diag, "events have no return type; write 'return;' to exit early");
                }
                return;
            }
            if (f->ret == T_VOID && st->u.ret.has_value) {
                diag_emit_range(s->diag, DIAG_ERROR, st->loc, st->loc.off + 6,
                    "function '%s' has no return type but returns a value", f->name);
                diag_hint(s->diag,
                    "either remove the value:  return;\nor declare the function with a return type:  TYPE %s(...)",
                    f->name);
                check_expr(s, st->u.ret.expr);
                return;
            }
            if (f->ret != T_VOID && !st->u.ret.has_value) {
                diag_emit_range(s->diag, DIAG_ERROR, st->loc, st->loc.off + 6,
                    "function '%s' must return a %s value", f->name, type_name(f->ret));
                return;
            }
            if (st->u.ret.has_value) {
                check_expr(s, st->u.ret.expr);
                if (!type_implicit_assignable(f->ret, st->u.ret.expr->type)) {
                    diag_emit_range(s->diag, DIAG_ERROR, st->u.ret.expr->loc, st->u.ret.expr->end_off,
                        "returning %s from function '%s' which expects %s",
                        type_name(st->u.ret.expr->type), f->name, type_name(f->ret));
                    if (type_explicit_castable(f->ret, st->u.ret.expr->type))
                        diag_hint(s->diag, "cast the result:  return (%s)expr;", type_name(f->ret));
                }
            }
            return;
        }
        case S_JUMP: {
            /* Track for later resolution. */
            LabelList *j = (LabelList*)xcalloc(1, sizeof(LabelList));
            j->name = xstrdup(st->u.jmp.label ? st->u.jmp.label : "?");
            j->loc = st->u.jmp.lbl_loc;
            j->next = s->jumps; s->jumps = j;
            return;
        }
        case S_LABEL: {
            /* Add to label list. Duplicates within same function are an error
             * detected at resolution time. */
            LabelList *l = (LabelList*)xcalloc(1, sizeof(LabelList));
            l->name = xstrdup(st->u.lbl.label ? st->u.lbl.label : "?");
            l->loc = st->loc;
            l->next = s->labels; s->labels = l;
            return;
        }
        case S_STATECHG: {
            if (!s->in_event) {
                diag_emit_range(s->diag, DIAG_ERROR, st->loc, st->loc.off + 5,
                    "'state' change is only allowed inside event handlers, not in user-defined functions");
                diag_hint(s->diag,
                    "to switch state from a function, return a value and have the caller's event do the state change");
                return;
            }
            const char *tgt = st->u.stc.state ? st->u.stc.state : "";
            int found = 0;
            for (int i = 0; i < s->ast->n_states; i++) {
                if (strcmp(s->ast->states[i].name, tgt) == 0) { found = 1; break; }
            }
            if (!found) {
                diag_emit_range(s->diag, DIAG_ERROR, st->u.stc.name_loc, st->u.stc.name_loc.off + 1,
                    "state '%s' is not declared in this script", tgt);
                suggest_state(s, tgt);
            }
            return;
        }
    }
}

static void resolve_labels_and_jumps(Sem *s) {
    /* Each jump must match exactly one label. Duplicates are reported. */
    /* Check duplicate labels */
    for (LabelList *a = s->labels; a; a = a->next) {
        for (LabelList *b = a->next; b; b = b->next) {
            if (strcmp(a->name, b->name) == 0) {
                diag_emit_range(s->diag, DIAG_ERROR, b->loc, b->loc.off + 1,
                    "duplicate label '@%s' in the same scope", a->name);
                diag_note(s->diag, a->loc, "previously defined here");
            }
        }
    }
    for (LabelList *j = s->jumps; j; j = j->next) {
        int found = 0;
        for (LabelList *l = s->labels; l; l = l->next) {
            if (strcmp(l->name, j->name) == 0) { found = 1; break; }
        }
        if (!found) {
            diag_emit_range(s->diag, DIAG_ERROR, j->loc, j->loc.off + 1,
                "no label '@%s' in scope to jump to", j->name);
            /* Suggest from labels list */
            int n = 0;
            for (LabelList *l = s->labels; l; l = l->next) n++;
            if (n > 0) {
                const char **names = (const char**)xmalloc(sizeof(char*) * n);
                int i = 0; for (LabelList *l = s->labels; l; l = l->next) names[i++] = l->name;
                const char *g = diag_did_you_mean(j->name, names, n, 2);
                if (g) diag_hint(s->diag, "did you mean '@%s'?", g);
                free(names);
            }
        }
    }
    /* Free lists */
    while (s->labels) { LabelList *n = s->labels->next; free(s->labels->name); free(s->labels); s->labels = n; }
    while (s->jumps)  { LabelList *n = s->jumps->next;  free(s->jumps->name);  free(s->jumps);  s->jumps  = n; }
}

/* --------------------------- Top-level checks --------------------------- */

static void check_globals(Sem *s) {
    for (int i = 0; i < s->ast->n_globals; i++) {
        GlobalVar *g = &s->ast->globals[i];
        Sym *existing = sym_lookup_local(&s->st, g->name);
        if (existing) {
            diag_emit_range(s->diag, DIAG_ERROR, g->loc, g->loc.off + 1,
                "redeclaration of global '%s'", g->name);
            diag_note(s->diag, existing->loc, "previously declared here");
            continue;
        }
        if (bi_lookup_const(g->name)) {
            diag_emit_range(s->diag, DIAG_WARNING, g->loc, g->loc.off + 1,
                "global '%s' shadows a built-in constant of the same name", g->name);
        }
        if (bi_lookup_fn(g->name)) {
            diag_emit_range(s->diag, DIAG_WARNING, g->loc, g->loc.off + 1,
                "global '%s' shadows a built-in function of the same name", g->name);
        }
        sym_define(&s->st, g->name, SYM_GLOBAL, g->type, g->loc);
    }
    /* Now type-check initializers (so that order-independent globals work). */
    for (int i = 0; i < s->ast->n_globals; i++) {
        GlobalVar *g = &s->ast->globals[i];
        if (!g->init) continue;
        check_expr(s, g->init);
        if (!type_implicit_assignable(g->type, g->init->type)) {
            diag_emit_range(s->diag, DIAG_ERROR, g->init->loc, g->init->end_off,
                "cannot initialize global '%s' (type %s) with %s",
                g->name, type_name(g->type), type_name(g->init->type));
            if (type_explicit_castable(g->type, g->init->type))
                diag_hint(s->diag, "an explicit cast is required:  (%s)expr", type_name(g->type));
        }
        /* Fold the initializer, then verify it is a constant expression.
         * LSL Mono rejects globals whose initialiser references variables
         * or calls functions. */
        fold_expr(&g->init, s->diag);
        if (!expr_is_constant(g->init)) {
            diag_emit_range(s->diag, DIAG_ERROR, g->init->loc, g->init->end_off,
                "global '%s' must be initialised with a constant expression",
                g->name);
            diag_hint(s->diag,
                "global initialisers may use literals, type casts on literals, "
                "and operators only — no function calls and no references to other variables");
            if (g->init->kind == E_CALL) {
                diag_hint(s->diag,
                    "move the call into state_entry() and assign to '%s' there",
                    g->name);
            } else if (g->init->kind == E_IDENT) {
                diag_hint(s->diag,
                    "LSL does not allow a global to reference another variable in its initialiser");
            }
        }
    }
}

static void check_functions_declare(Sem *s) {
    for (int i = 0; i < s->ast->n_funcs; i++) {
        FuncDecl *f = &s->ast->funcs[i];
        Sym *existing = sym_lookup_local(&s->st, f->name);
        if (existing) {
            diag_emit_range(s->diag, DIAG_ERROR, f->loc, f->loc.off + 1,
                "redeclaration of function '%s'", f->name);
            diag_note(s->diag, existing->loc, "previously declared here");
            continue;
        }
        if (bi_lookup_fn(f->name)) {
            diag_emit_range(s->diag, DIAG_WARNING, f->loc, f->loc.off + 1,
                "user function '%s' has the same name as a built-in", f->name);
            diag_hint(s->diag, "calls to '%s' will use your definition; consider renaming to avoid confusion", f->name);
        }
        Sym *fs = sym_define(&s->st, f->name, SYM_FUNC, f->ret, f->loc);
        if (fs) {
            fs->n_params = f->n_params;
            for (int p = 0; p < f->n_params && p < 16; p++) fs->params[p] = f->params[p].type;
        }
    }
}

static void check_states_declare(Sem *s) {
    int saw_default = 0;
    for (int i = 0; i < s->ast->n_states; i++) {
        StateDecl *st = &s->ast->states[i];
        Sym *existing = sym_lookup_local(&s->st, st->name);
        if (existing) {
            diag_emit_range(s->diag, DIAG_ERROR, st->loc, st->loc.off + 1,
                "redeclaration of state '%s'", st->name);
            diag_note(s->diag, existing->loc, "previously declared here");
            continue;
        }
        if (st->is_default) saw_default = 1;
        sym_define(&s->st, st->name, SYM_STATE, T_VOID, st->loc);
    }
    if (!saw_default && s->ast->n_states > 0) {
        SrcLoc l = s->ast->states[0].loc;
        diag_emit(s->diag, DIAG_ERROR, l,
            "this script has no 'default' state");
        diag_hint(s->diag, "every LSL script must define a 'default { ... }' state");
    } else if (s->ast->n_states == 0) {
        SrcLoc l = {0}; l.line = 1; l.col = 1;
        diag_emit(s->diag, DIAG_ERROR, l,
            "no 'default' state found");
        diag_hint(s->diag, "an LSL script must contain at least:\n    default { state_entry() { } }");
    }
}

static void check_function_body(Sem *s, FuncDecl *f) {
    s->cur_func = f;
    s->cur_event = NULL;
    s->in_event = 0;
    sym_push_scope(&s->st);
    for (int i = 0; i < f->n_params; i++) {
        Param *p = &f->params[i];
        Sym *ex = sym_lookup_local(&s->st, p->name);
        if (ex) {
            diag_emit_range(s->diag, DIAG_ERROR, p->loc, p->loc.off + 1,
                "duplicate parameter name '%s' in function '%s'", p->name, f->name);
            diag_note(s->diag, ex->loc, "previously declared here");
        } else {
            sym_define(&s->st, p->name, SYM_PARAM, p->type, p->loc);
        }
    }
    s->labels = NULL; s->jumps = NULL;
    if (f->body) check_block(s, f->body);
    resolve_labels_and_jumps(s);
    sym_pop_scope(&s->st);
    s->cur_func = NULL;
}

static void check_event(Sem *s, EventDecl *e) {
    s->cur_func = NULL;
    s->cur_event = e;
    s->in_event = 1;
    /* Validate event name and signature */
    const BuiltinEvent *be = bi_lookup_event(e->name);
    if (!be) {
        diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->loc.off + 1,
            "unknown event handler '%s'", e->name);
        suggest_event(s, e->name);
    } else {
        /* Find an arity-compatible variant. We allow multiple overloaded
         * entries by linear scan. */
        int matched = 0;
        for (int i = 0; i < BI_EVENT_N; i++) {
            if (strcmp(BI_EVENT[i].name, e->name) == 0 &&
                BI_EVENT[i].n_params == e->n_params) {
                matched = 1; be = &BI_EVENT[i]; break;
            }
        }
        if (!matched) {
            diag_emit_range(s->diag, DIAG_ERROR, e->loc, e->loc.off + 1,
                "event '%s' takes %d parameter%s, you wrote %d",
                e->name, be->n_params, be->n_params == 1 ? "" : "s", e->n_params);
        } else {
            for (int i = 0; i < e->n_params; i++) {
                if (be->params[i] == T_VOID) continue;
                if (e->params[i].type != be->params[i]) {
                    diag_emit_range(s->diag, DIAG_ERROR, e->params[i].loc, e->params[i].loc.off + 1,
                        "event '%s' parameter %d ('%s') has type %s, expected %s",
                        e->name, i + 1, be->param_names[i] ? be->param_names[i] : "?",
                        type_name(e->params[i].type), type_name(be->params[i]));
                    char sigbuf[256]; sigbuf[0] = '\0';
                    int o = 0;
                    for (int k = 0; k < be->n_params; k++) {
                        o += snprintf(sigbuf + o, sizeof(sigbuf) - o,
                            "%s%s %s", k ? ", " : "",
                            type_name(be->params[k]),
                            be->param_names[k] ? be->param_names[k] : "");
                        if (o >= (int)sizeof(sigbuf)) break;
                    }
                    diag_hint(s->diag, "the canonical signature is:  %s(%s)", e->name, sigbuf);
                }
            }
        }
    }
    sym_push_scope(&s->st);
    for (int i = 0; i < e->n_params; i++) {
        Param *p = &e->params[i];
        Sym *ex = sym_lookup_local(&s->st, p->name);
        if (ex) {
            diag_emit_range(s->diag, DIAG_ERROR, p->loc, p->loc.off + 1,
                "duplicate parameter name '%s' in event '%s'", p->name, e->name);
            diag_note(s->diag, ex->loc, "previously declared here");
        } else {
            sym_define(&s->st, p->name, SYM_PARAM, p->type, p->loc);
        }
    }
    s->labels = NULL; s->jumps = NULL;
    if (e->body) check_block(s, e->body);
    resolve_labels_and_jumps(s);
    sym_pop_scope(&s->st);
    s->cur_event = NULL;
    s->in_event = 0;
}

static void check_state(Sem *s, StateDecl *st) {
    s->cur_state = st;
    /* duplicate events */
    for (int i = 0; i < st->n_events; i++) {
        for (int j = i + 1; j < st->n_events; j++) {
            if (strcmp(st->events[i].name, st->events[j].name) == 0) {
                diag_emit_range(s->diag, DIAG_ERROR, st->events[j].loc, st->events[j].loc.off + 1,
                    "duplicate event handler '%s' in state '%s'", st->events[j].name, st->name);
                diag_note(s->diag, st->events[i].loc, "previously defined here");
            }
        }
    }
    for (int i = 0; i < st->n_events; i++) {
        check_event(s, &st->events[i]);
    }
    s->cur_state = NULL;
}

int sem_check(ScriptAST *ast, DiagCtx *d) {
    Sem s; memset(&s, 0, sizeof s);
    s.diag = d; s.ast = ast;
    sym_init(&s.st, d);

    /* 1. Declare globals, functions, states (forward references) */
    check_globals(&s);
    check_functions_declare(&s);
    check_states_declare(&s);

    /* 2. Type-check function bodies */
    for (int i = 0; i < ast->n_funcs; i++)  check_function_body(&s, &ast->funcs[i]);

    /* 3. Type-check states */
    for (int i = 0; i < ast->n_states; i++) check_state(&s, &ast->states[i]);

    /* 4. Optional: unused warnings under -Wall */
    if (d->wall) {
        Scope *sc = s.st.global;
        for (Sym *cur = sc->list; cur; cur = cur->next_in_scope) {
            if (cur->kind == SYM_GLOBAL && !cur->used) {
                diag_emit(d, DIAG_WARNING, cur->loc, "unused global variable '%s'", cur->name);
            } else if (cur->kind == SYM_FUNC && !cur->used) {
                /* Functions can be "unused" if not called — warn. */
                /* Skip if name starts with "_" to allow opt-out. */
                if (cur->name[0] != '_')
                    diag_emit(d, DIAG_WARNING, cur->loc, "unused function '%s'", cur->name);
            }
        }
    }

    sym_free(&s.st);
    return d->errors == 0;
}
