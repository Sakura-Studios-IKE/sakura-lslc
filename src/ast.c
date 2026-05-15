/* ast.c - AST cleanup helpers */
#include "lsl.h"
#include <stdlib.h>

void ast_free_expr(Expr *e) {
    if (!e) return;
    switch (e->kind) {
        case E_INT_LIT:
        case E_FLOAT_LIT:
            break;
        case E_STRING_LIT: free(e->u.slit); break;
        case E_IDENT:      free(e->u.ident); break;
        case E_VECTOR_LIT:
        case E_ROT_LIT:
            ast_free_expr(e->u.vec.x);
            ast_free_expr(e->u.vec.y);
            ast_free_expr(e->u.vec.z);
            ast_free_expr(e->u.vec.w);
            break;
        case E_LIST_LIT:
            for (int i = 0; i < e->u.list.n; i++) ast_free_expr(e->u.list.items[i]);
            free(e->u.list.items);
            break;
        case E_CALL:
            free(e->u.call.name);
            for (int i = 0; i < e->u.call.n_args; i++) ast_free_expr(e->u.call.args[i]);
            free(e->u.call.args);
            break;
        case E_MEMBER:
            ast_free_expr(e->u.memb.base);
            break;
        case E_CAST:
            ast_free_expr(e->u.cast.inner);
            break;
        case E_UNARY:
            ast_free_expr(e->u.un.inner);
            break;
        case E_POSTFIX:
            ast_free_expr(e->u.post.inner);
            break;
        case E_BINARY:
            ast_free_expr(e->u.bin.l);
            ast_free_expr(e->u.bin.r);
            break;
        case E_ASSIGN:
            ast_free_expr(e->u.asn.l);
            ast_free_expr(e->u.asn.r);
            break;
    }
    free(e);
}

void ast_free_stmt(Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case S_EMPTY: break;
        case S_EXPR: ast_free_expr(s->u.expr); break;
        case S_DECL:
            free(s->u.decl.name);
            ast_free_expr(s->u.decl.init);
            break;
        case S_BLOCK:
            for (int i = 0; i < s->u.block.n; i++) ast_free_stmt(s->u.block.stmts[i]);
            free(s->u.block.stmts);
            break;
        case S_IF:
            ast_free_expr(s->u.if_s.cond);
            ast_free_stmt(s->u.if_s.then_s);
            ast_free_stmt(s->u.if_s.else_s);
            break;
        case S_WHILE:
            ast_free_expr(s->u.while_s.cond);
            ast_free_stmt(s->u.while_s.body);
            break;
        case S_DO:
            ast_free_stmt(s->u.do_s.body);
            ast_free_expr(s->u.do_s.cond);
            break;
        case S_FOR:
            for (int i = 0; i < s->u.for_s.n_init; i++) ast_free_expr(s->u.for_s.init[i]);
            free(s->u.for_s.init);
            ast_free_expr(s->u.for_s.cond);
            for (int i = 0; i < s->u.for_s.n_post; i++) ast_free_expr(s->u.for_s.post[i]);
            free(s->u.for_s.post);
            ast_free_stmt(s->u.for_s.body);
            break;
        case S_RETURN: ast_free_expr(s->u.ret.expr); break;
        case S_JUMP:   free(s->u.jmp.label); break;
        case S_LABEL:  free(s->u.lbl.label); break;
        case S_STATECHG: free(s->u.stc.state); break;
    }
    free(s);
}

static void free_event(EventDecl *e) {
    free(e->name);
    for (int i = 0; i < e->n_params; i++) free(e->params[i].name);
    free(e->params);
    ast_free_stmt(e->body);
}

static void free_state(StateDecl *st) {
    free(st->name);
    for (int i = 0; i < st->n_events; i++) free_event(&st->events[i]);
    free(st->events);
}

static void free_func(FuncDecl *f) {
    free(f->name);
    for (int i = 0; i < f->n_params; i++) free(f->params[i].name);
    free(f->params);
    ast_free_stmt(f->body);
}

void ast_free_script(ScriptAST *s) {
    if (!s) return;
    for (int i = 0; i < s->n_globals; i++) {
        free(s->globals[i].name);
        ast_free_expr(s->globals[i].init);
    }
    free(s->globals);
    for (int i = 0; i < s->n_funcs; i++) free_func(&s->funcs[i]);
    free(s->funcs);
    for (int i = 0; i < s->n_states; i++) free_state(&s->states[i]);
    free(s->states);
}
