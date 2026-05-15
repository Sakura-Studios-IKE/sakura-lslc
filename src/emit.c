/* emit.c - serialise a typed LSL AST to the SLBC bytecode format.
 *
 * Format (all integers little-endian):
 *
 *   header
 *     u8[5]   magic  "SLBC\0"
 *     u32     version (1)
 *     u32     flags  (bit 0 = LSO target)
 *
 *   string pool
 *     u32     n_strings
 *     repeat: u32 length, bytes[length]      (NOT NUL-terminated)
 *
 *   globals
 *     u32     n_globals
 *     repeat: u8 type, u32 name_idx, u8 has_init, [expr if has_init]
 *
 *   functions
 *     u32     n_funcs
 *     repeat: u32 name_idx, u32 line_no, u8 ret_type, u8 has_return_type,
 *             u8 n_params, n_params*(u8 type, u32 name_idx), stmt body
 *
 *   states
 *     u32     n_states
 *     repeat: u32 name_idx, u32 line_no, u8 is_default,
 *             u32 n_events,
 *             n_events * (u32 name_idx, u32 line_no, u8 n_params,
 *                         n_params*(u8 type, u32 name_idx), stmt body)
 *
 *   stmt:
 *     u8 tag
 *     u32 line_no            (0 if unknown; new in SLBC v2)
 *     payload...
 *
 *   Expression / Statement nodes are tagged trees. Tags use the existing
 *   ExprKind / StmtKind enum values verbatim — see lsl.h. Layout:
 *
 *   expr:
 *     u8 tag
 *     u8 type        (LSL type tag, T_*)
 *     u8 is_lvalue
 *     payload...
 *
 *   stmt:
 *     u8 tag
 *     payload...
 *
 * Strings inside expressions go through the pool. Identifiers do too.
 * SrcLoc info is dropped to keep files small; runtime diagnostics from
 * slemu mention only logical positions ("function foo", "event timer in
 * state default") rather than line numbers.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include "lsl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    FILE *f;
    /* String pool: collected on a first pass, then written once. */
    char **strs;
    int    n_strs;
    int    cap_strs;
    DiagCtx *diag;
} Emit;

static int pool_intern(Emit *e, const char *s) {
    if (!s) s = "";
    for (int i = 0; i < e->n_strs; i++)
        if (strcmp(e->strs[i], s) == 0) return i;
    if (e->n_strs == e->cap_strs) {
        e->cap_strs = e->cap_strs ? e->cap_strs * 2 : 16;
        e->strs = (char**)xrealloc(e->strs, sizeof(char*) * e->cap_strs);
    }
    e->strs[e->n_strs] = xstrdup(s);
    return e->n_strs++;
}

/* ----------- low-level writers --------------------------------------- */
static void w_bytes(Emit *e, const void *p, size_t n) { fwrite(p, 1, n, e->f); }
static void w_u8 (Emit *e, uint8_t  v) { w_bytes(e, &v, 1); }
static void w_u32(Emit *e, uint32_t v) {
    uint8_t b[4] = { v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF };
    w_bytes(e, b, 4);
}
static void w_i64(Emit *e, int64_t v) {
    uint64_t u = (uint64_t)v;
    uint8_t b[8];
    for (int i = 0; i < 8; i++) { b[i] = u & 0xFF; u >>= 8; }
    w_bytes(e, b, 8);
}
static void w_f64(Emit *e, double v) {
    union { double d; uint64_t u; } x; x.d = v;
    uint64_t u = x.u;
    uint8_t b[8];
    for (int i = 0; i < 8; i++) { b[i] = u & 0xFF; u >>= 8; }
    w_bytes(e, b, 8);
}
static void w_str_idx(Emit *e, const char *s) { w_u32(e, (uint32_t)pool_intern(e, s)); }

/* ----------- forward decls ------------------------------------------- */
static void emit_expr(Emit *e, Expr *x);
static void emit_stmt(Emit *e, Stmt *s);

/* ----------- pool collection (pass 1) -------------------------------- */
static void collect_expr(Emit *e, Expr *x);
static void collect_stmt(Emit *e, Stmt *s);

static void collect_expr(Emit *e, Expr *x) {
    if (!x) return;
    switch (x->kind) {
        case E_INT_LIT: case E_FLOAT_LIT: break;
        case E_STRING_LIT: pool_intern(e, x->u.slit ? x->u.slit : ""); break;
        case E_IDENT:      pool_intern(e, x->u.ident ? x->u.ident : ""); break;
        case E_VECTOR_LIT: case E_ROT_LIT:
            collect_expr(e, x->u.vec.x); collect_expr(e, x->u.vec.y);
            collect_expr(e, x->u.vec.z); collect_expr(e, x->u.vec.w);
            break;
        case E_LIST_LIT:
            for (int i = 0; i < x->u.list.n; i++) collect_expr(e, x->u.list.items[i]);
            break;
        case E_CALL:
            pool_intern(e, x->u.call.name ? x->u.call.name : "");
            for (int i = 0; i < x->u.call.n_args; i++) collect_expr(e, x->u.call.args[i]);
            break;
        case E_MEMBER: collect_expr(e, x->u.memb.base); break;
        case E_CAST:   collect_expr(e, x->u.cast.inner); break;
        case E_UNARY:  collect_expr(e, x->u.un.inner); break;
        case E_POSTFIX:collect_expr(e, x->u.post.inner); break;
        case E_BINARY: collect_expr(e, x->u.bin.l); collect_expr(e, x->u.bin.r); break;
        case E_ASSIGN: collect_expr(e, x->u.asn.l); collect_expr(e, x->u.asn.r); break;
    }
}

static void collect_stmt(Emit *e, Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case S_EMPTY: break;
        case S_EXPR: collect_expr(e, s->u.expr); break;
        case S_DECL:
            pool_intern(e, s->u.decl.name ? s->u.decl.name : "");
            collect_expr(e, s->u.decl.init);
            break;
        case S_BLOCK:
            for (int i = 0; i < s->u.block.n; i++) collect_stmt(e, s->u.block.stmts[i]);
            break;
        case S_IF:
            collect_expr(e, s->u.if_s.cond);
            collect_stmt(e, s->u.if_s.then_s);
            collect_stmt(e, s->u.if_s.else_s);
            break;
        case S_WHILE: collect_expr(e, s->u.while_s.cond); collect_stmt(e, s->u.while_s.body); break;
        case S_DO:    collect_stmt(e, s->u.do_s.body); collect_expr(e, s->u.do_s.cond); break;
        case S_FOR:
            for (int i = 0; i < s->u.for_s.n_init; i++) collect_expr(e, s->u.for_s.init[i]);
            collect_expr(e, s->u.for_s.cond);
            for (int i = 0; i < s->u.for_s.n_post; i++) collect_expr(e, s->u.for_s.post[i]);
            collect_stmt(e, s->u.for_s.body);
            break;
        case S_RETURN: collect_expr(e, s->u.ret.expr); break;
        case S_JUMP:   pool_intern(e, s->u.jmp.label ? s->u.jmp.label : ""); break;
        case S_LABEL:  pool_intern(e, s->u.lbl.label ? s->u.lbl.label : ""); break;
        case S_STATECHG: pool_intern(e, s->u.stc.state ? s->u.stc.state : ""); break;
    }
}

/* ----------- write expressions (pass 2) ------------------------------ */
static void emit_expr(Emit *e, Expr *x) {
    if (!x) { w_u8(e, 0xFF); return; }   /* sentinel for NULL */
    w_u8(e, (uint8_t)x->kind);
    w_u8(e, (uint8_t)x->type);
    w_u8(e, (uint8_t)x->is_lvalue);
    switch (x->kind) {
        case E_INT_LIT:   w_i64(e, x->u.ilit); break;
        case E_FLOAT_LIT: w_f64(e, x->u.flit); break;
        case E_STRING_LIT:w_str_idx(e, x->u.slit ? x->u.slit : ""); break;
        case E_IDENT:     w_str_idx(e, x->u.ident ? x->u.ident : ""); break;
        case E_VECTOR_LIT:
            emit_expr(e, x->u.vec.x); emit_expr(e, x->u.vec.y); emit_expr(e, x->u.vec.z);
            break;
        case E_ROT_LIT:
            emit_expr(e, x->u.vec.x); emit_expr(e, x->u.vec.y);
            emit_expr(e, x->u.vec.z); emit_expr(e, x->u.vec.w);
            break;
        case E_LIST_LIT:
            w_u32(e, (uint32_t)x->u.list.n);
            for (int i = 0; i < x->u.list.n; i++) emit_expr(e, x->u.list.items[i]);
            break;
        case E_CALL:
            w_str_idx(e, x->u.call.name);
            w_u32(e, (uint32_t)x->u.call.n_args);
            for (int i = 0; i < x->u.call.n_args; i++) emit_expr(e, x->u.call.args[i]);
            break;
        case E_MEMBER:
            emit_expr(e, x->u.memb.base);
            w_u8(e, (uint8_t)x->u.memb.member);
            break;
        case E_CAST:
            w_u8(e, (uint8_t)x->u.cast.to);
            emit_expr(e, x->u.cast.inner);
            break;
        case E_UNARY:
            w_u8(e, (uint8_t)x->u.un.op);
            emit_expr(e, x->u.un.inner);
            break;
        case E_POSTFIX:
            w_u8(e, (uint8_t)x->u.post.op);
            emit_expr(e, x->u.post.inner);
            break;
        case E_BINARY:
            w_u8(e, (uint8_t)x->u.bin.op);
            emit_expr(e, x->u.bin.l);
            emit_expr(e, x->u.bin.r);
            break;
        case E_ASSIGN:
            w_u8(e, (uint8_t)x->u.asn.op);
            emit_expr(e, x->u.asn.l);
            emit_expr(e, x->u.asn.r);
            break;
    }
}

static void emit_stmt(Emit *e, Stmt *s) {
    if (!s) { w_u8(e, 0xFF); return; }
    w_u8(e, (uint8_t)s->kind);
    w_u32(e, (uint32_t)(s->loc.line > 0 ? s->loc.line : 0));
    switch (s->kind) {
        case S_EMPTY: break;
        case S_EXPR: emit_expr(e, s->u.expr); break;
        case S_DECL:
            w_u8(e, (uint8_t)s->u.decl.t);
            w_str_idx(e, s->u.decl.name ? s->u.decl.name : "");
            w_u8(e, s->u.decl.init ? 1 : 0);
            if (s->u.decl.init) emit_expr(e, s->u.decl.init);
            break;
        case S_BLOCK:
            w_u32(e, (uint32_t)s->u.block.n);
            for (int i = 0; i < s->u.block.n; i++) emit_stmt(e, s->u.block.stmts[i]);
            break;
        case S_IF:
            emit_expr(e, s->u.if_s.cond);
            emit_stmt(e, s->u.if_s.then_s);
            w_u8(e, s->u.if_s.else_s ? 1 : 0);
            if (s->u.if_s.else_s) emit_stmt(e, s->u.if_s.else_s);
            break;
        case S_WHILE:
            emit_expr(e, s->u.while_s.cond);
            emit_stmt(e, s->u.while_s.body);
            break;
        case S_DO:
            emit_stmt(e, s->u.do_s.body);
            emit_expr(e, s->u.do_s.cond);
            break;
        case S_FOR:
            w_u8(e, (uint8_t)s->u.for_s.n_init);
            for (int i = 0; i < s->u.for_s.n_init; i++) emit_expr(e, s->u.for_s.init[i]);
            w_u8(e, s->u.for_s.cond ? 1 : 0);
            if (s->u.for_s.cond) emit_expr(e, s->u.for_s.cond);
            w_u8(e, (uint8_t)s->u.for_s.n_post);
            for (int i = 0; i < s->u.for_s.n_post; i++) emit_expr(e, s->u.for_s.post[i]);
            emit_stmt(e, s->u.for_s.body);
            break;
        case S_RETURN:
            w_u8(e, s->u.ret.has_value ? 1 : 0);
            if (s->u.ret.has_value) emit_expr(e, s->u.ret.expr);
            break;
        case S_JUMP:   w_str_idx(e, s->u.jmp.label ? s->u.jmp.label : ""); break;
        case S_LABEL:  w_str_idx(e, s->u.lbl.label ? s->u.lbl.label : ""); break;
        case S_STATECHG: w_str_idx(e, s->u.stc.state ? s->u.stc.state : ""); break;
    }
}

/* ----------- top-level ----------------------------------------------- */
int emit_bytecode(ScriptAST *ast, const char *out_path, int lso, DiagCtx *diag) {
    Emit e; memset(&e, 0, sizeof e);
    e.diag = diag;

    /* Pass 1: collect strings */
    for (int i = 0; i < ast->n_globals; i++) {
        pool_intern(&e, ast->globals[i].name ? ast->globals[i].name : "");
        collect_expr(&e, ast->globals[i].init);
    }
    for (int i = 0; i < ast->n_funcs; i++) {
        pool_intern(&e, ast->funcs[i].name ? ast->funcs[i].name : "");
        for (int j = 0; j < ast->funcs[i].n_params; j++)
            pool_intern(&e, ast->funcs[i].params[j].name ? ast->funcs[i].params[j].name : "");
        collect_stmt(&e, ast->funcs[i].body);
    }
    for (int i = 0; i < ast->n_states; i++) {
        pool_intern(&e, ast->states[i].name ? ast->states[i].name : "");
        for (int j = 0; j < ast->states[i].n_events; j++) {
            EventDecl *ev = &ast->states[i].events[j];
            pool_intern(&e, ev->name ? ev->name : "");
            for (int k = 0; k < ev->n_params; k++)
                pool_intern(&e, ev->params[k].name ? ev->params[k].name : "");
            collect_stmt(&e, ev->body);
        }
    }

    /* Open output */
    e.f = fopen(out_path, "wb");
    if (!e.f) {
        SrcLoc l = {0}; l.file = ast->file; l.line = 1; l.col = 1;
        diag_emit(diag, DIAG_FATAL, l, "cannot write '%s'", out_path);
        for (int i = 0; i < e.n_strs; i++) free(e.strs[i]);
        free(e.strs);
        return 0;
    }

    /* Header */
    w_bytes(&e, "SLBC\0", 5);
    w_u32(&e, 2);                            /* version (2 adds line numbers) */
    w_u32(&e, lso ? 1u : 0u);                /* flags */

    /* String pool */
    w_u32(&e, (uint32_t)e.n_strs);
    for (int i = 0; i < e.n_strs; i++) {
        size_t len = strlen(e.strs[i]);
        w_u32(&e, (uint32_t)len);
        w_bytes(&e, e.strs[i], len);
    }

    /* Globals */
    w_u32(&e, (uint32_t)ast->n_globals);
    for (int i = 0; i < ast->n_globals; i++) {
        GlobalVar *g = &ast->globals[i];
        w_u8(&e, (uint8_t)g->type);
        w_str_idx(&e, g->name ? g->name : "");
        w_u8(&e, g->init ? 1 : 0);
        if (g->init) emit_expr(&e, g->init);
    }

    /* Functions */
    w_u32(&e, (uint32_t)ast->n_funcs);
    for (int i = 0; i < ast->n_funcs; i++) {
        FuncDecl *f = &ast->funcs[i];
        w_str_idx(&e, f->name ? f->name : "");
        w_u32(&e, (uint32_t)(f->loc.line > 0 ? f->loc.line : 0));
        w_u8(&e, (uint8_t)f->ret);
        w_u8(&e, (uint8_t)f->has_return_type);
        w_u8(&e, (uint8_t)f->n_params);
        for (int j = 0; j < f->n_params; j++) {
            w_u8(&e, (uint8_t)f->params[j].type);
            w_str_idx(&e, f->params[j].name ? f->params[j].name : "");
        }
        emit_stmt(&e, f->body);
    }

    /* States */
    w_u32(&e, (uint32_t)ast->n_states);
    for (int i = 0; i < ast->n_states; i++) {
        StateDecl *st = &ast->states[i];
        w_str_idx(&e, st->name ? st->name : "");
        w_u32(&e, (uint32_t)(st->loc.line > 0 ? st->loc.line : 0));
        w_u8(&e, (uint8_t)st->is_default);
        w_u32(&e, (uint32_t)st->n_events);
        for (int j = 0; j < st->n_events; j++) {
            EventDecl *ev = &st->events[j];
            w_str_idx(&e, ev->name ? ev->name : "");
            w_u32(&e, (uint32_t)(ev->loc.line > 0 ? ev->loc.line : 0));
            w_u8(&e, (uint8_t)ev->n_params);
            for (int k = 0; k < ev->n_params; k++) {
                w_u8(&e, (uint8_t)ev->params[k].type);
                w_str_idx(&e, ev->params[k].name ? ev->params[k].name : "");
            }
            emit_stmt(&e, ev->body);
        }
    }

    fclose(e.f);
    for (int i = 0; i < e.n_strs; i++) free(e.strs[i]);
    free(e.strs);
    return 1;
}
