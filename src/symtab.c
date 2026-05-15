/* symtab.c - lexical scope and symbol-table management */
#include "lsl.h"
#include <stdlib.h>
#include <string.h>

static unsigned hash_str(const char *s) {
    unsigned h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}

static Scope *new_scope(Scope *parent) {
    Scope *s = (Scope*)xcalloc(1, sizeof(Scope));
    s->parent = parent;
    s->depth = parent ? parent->depth + 1 : 0;
    s->nbuckets = 64;
    s->buckets = (Sym**)xcalloc((size_t)s->nbuckets, sizeof(Sym*));
    return s;
}

void sym_init(SymTab *s, DiagCtx *d) {
    memset(s, 0, sizeof *s);
    s->global = new_scope(NULL);
    s->cur = s->global;
    s->diag = d;
}

static void free_scope(Scope *sc) {
    Sym *cur = sc->list;
    while (cur) {
        Sym *next = cur->next_in_scope;
        free(cur->name);
        free(cur);
        cur = next;
    }
    free(sc->buckets);
    free(sc);
}

void sym_free(SymTab *s) {
    /* Caller must pop all non-global scopes; we always free global last. */
    while (s->cur && s->cur != s->global) {
        Scope *p = s->cur->parent;
        free_scope(s->cur);
        s->cur = p;
    }
    if (s->global) { free_scope(s->global); s->global = NULL; s->cur = NULL; }
}

void sym_push_scope(SymTab *s) {
    s->cur = new_scope(s->cur);
}
void sym_pop_scope(SymTab *s) {
    if (!s->cur || s->cur == s->global) return;
    Scope *p = s->cur->parent;
    free_scope(s->cur);
    s->cur = p;
}

static Sym *bucket_lookup(Scope *sc, const char *name) {
    unsigned h = hash_str(name) % (unsigned)sc->nbuckets;
    Sym *c = sc->buckets[h];
    while (c) {
        if (strcmp(c->name, name) == 0) return c;
        c = c->next_in_bucket;
    }
    return NULL;
}

Sym *sym_define(SymTab *s, const char *name, SymKind k, TypeKind t, SrcLoc loc) {
    Sym *existing = bucket_lookup(s->cur, name);
    if (existing) {
        /* Duplicate in same scope — caller wants to know. */
        return existing;
    }
    Sym *n = (Sym*)xcalloc(1, sizeof(Sym));
    n->name = xstrdup(name);
    n->kind = k;
    n->type = t;
    n->scope_depth = s->cur->depth;
    n->loc = loc;
    unsigned h = hash_str(name) % (unsigned)s->cur->nbuckets;
    n->next_in_bucket = s->cur->buckets[h];
    s->cur->buckets[h] = n;
    n->next_in_scope = s->cur->list;
    s->cur->list = n;
    return n;
}

Sym *sym_lookup_local(SymTab *s, const char *name) {
    return bucket_lookup(s->cur, name);
}

Sym *sym_lookup_global(SymTab *s, const char *name) {
    return bucket_lookup(s->global, name);
}

Sym *sym_lookup(SymTab *s, const char *name) {
    Scope *sc = s->cur;
    while (sc) {
        Sym *r = bucket_lookup(sc, name);
        if (r) return r;
        sc = sc->parent;
    }
    return NULL;
}
