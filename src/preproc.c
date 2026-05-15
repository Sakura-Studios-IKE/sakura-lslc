/* preproc.c - C-style preprocessor for LSL.
 *
 * Compatible (in syntax) with the Firestorm viewer's LSL preprocessor and
 * the OpenSim "lslc" macro tool: the goal is that a script that works
 * under those tools also works here.
 *
 * Supported directives:
 *   #include "path"        — relative to including file
 *   #include <path>        — searched along -I paths
 *   #define NAME [body]
 *   #define NAME(args) body
 *   #undef NAME
 *   #ifdef NAME / #ifndef NAME
 *   #if defined(NAME)              (only this form of #if, plus #if 1/0)
 *   #else / #elif defined(NAME) / #endif
 *   #error "msg"   #warning "msg"
 *   #line N "path"          — recognised but not user-facing
 *   #pragma                 — ignored
 *
 * Output is a single buffer where every #line N "path" marker tells the
 * lexer to reset its tracking. Skipped (false) conditional branches are
 * replaced with blank lines so line numbers don't shift inside a file.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include "lsl.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

/* -------------------- dynamic string builder ---------------------------- */
typedef struct { char *buf; size_t len, cap; } Sbuf;
static void sb_init(Sbuf *s) { s->buf = NULL; s->len = 0; s->cap = 0; }
static void sb_reserve(Sbuf *s, size_t need) {
    if (s->len + need + 1 > s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 256;
        while (nc < s->len + need + 1) nc *= 2;
        s->buf = (char*)xrealloc(s->buf, nc);
        s->cap = nc;
    }
}
static void sb_putc(Sbuf *s, char c) { sb_reserve(s, 1); s->buf[s->len++] = c; s->buf[s->len] = '\0'; }
static void sb_puts(Sbuf *s, const char *t) { size_t l = strlen(t); sb_reserve(s, l); memcpy(s->buf+s->len, t, l); s->len += l; s->buf[s->len] = '\0'; }
static void sb_putn(Sbuf *s, const char *t, size_t n) { sb_reserve(s, n); memcpy(s->buf+s->len, t, n); s->len += n; s->buf[s->len] = '\0'; }
static void sb_printf(Sbuf *s, const char *fmt, ...) {
    char tmp[512];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    sb_putn(s, tmp, (size_t)n);
}

/* -------------------- include-path helpers ------------------------------ */
void preproc_init(Preproc *p, DiagCtx *d) {
    memset(p, 0, sizeof *p);
    p->diag = d;
    p->max_include_depth = 32;
}
void preproc_free(Preproc *p) {
    PpMacro *m = p->macros;
    while (m) {
        PpMacro *n = m->next;
        free(m->name); free(m->body);
        for (int i = 0; i < m->n_params; i++) free(m->params[i]);
        free(m->params);
        free(m);
        m = n;
    }
    for (int i = 0; i < p->n_include_paths; i++) free(p->include_paths[i]);
    free(p->include_paths);
}
void preproc_add_include_path(Preproc *p, const char *path) {
    p->include_paths = (char**)xrealloc(p->include_paths, sizeof(char*) * (p->n_include_paths + 1));
    p->include_paths[p->n_include_paths++] = xstrdup(path);
}

static PpMacro *find_macro(Preproc *p, const char *name) {
    for (PpMacro *m = p->macros; m; m = m->next)
        if (strcmp(m->name, name) == 0) return m;
    return NULL;
}

static void add_macro(Preproc *p, const char *name, int is_func, int n_params,
                      char **params, const char *body) {
    PpMacro *old = find_macro(p, name);
    if (old) {
        free(old->body); old->body = xstrdup(body ? body : "");
        old->is_func = is_func;
        for (int i = 0; i < old->n_params; i++) free(old->params[i]);
        free(old->params);
        old->params = params; old->n_params = n_params;
        return;
    }
    PpMacro *m = (PpMacro*)xcalloc(1, sizeof *m);
    m->name = xstrdup(name);
    m->is_func = is_func;
    m->n_params = n_params;
    m->params = params;
    m->body = xstrdup(body ? body : "");
    m->next = p->macros;
    p->macros = m;
}

void preproc_define_cli(Preproc *p, const char *spec) {
    const char *eq = strchr(spec, '=');
    if (eq) {
        size_t nlen = (size_t)(eq - spec);
        char *name = xstrndup(spec, nlen);
        add_macro(p, name, 0, 0, NULL, eq + 1);
        free(name);
    } else {
        add_macro(p, spec, 0, 0, NULL, "1");
    }
}

/* -------------------- text helpers -------------------------------------- */
static int is_id_start(int c) { return c == '_' || isalpha(c); }
static int is_id_cont (int c) { return c == '_' || isalnum(c); }

static char *dirname_of(const char *path) {
    const char *s = strrchr(path, '/');
#ifdef _WIN32
    const char *t = strrchr(path, '\\');
    if (t && (!s || t > s)) s = t;
#endif
    if (!s) return xstrdup(".");
    return xstrndup(path, (size_t)(s - path));
}

static char *path_join(const char *dir, const char *file) {
    size_t ld = strlen(dir), lf = strlen(file);
    char *r = (char*)xmalloc(ld + 1 + lf + 1);
    memcpy(r, dir, ld);
    r[ld] = PATH_SEP;
    memcpy(r + ld + 1, file, lf + 1);
    return r;
}

static int file_exists(const char *p) {
    FILE *f = fopen(p, "rb"); if (!f) return 0; fclose(f); return 1;
}

static char *resolve_include(Preproc *p, const char *base_dir, const char *spec, int system) {
    if (!system) {
        char *cand = path_join(base_dir, spec);
        if (file_exists(cand)) return cand;
        free(cand);
    }
    for (int i = 0; i < p->n_include_paths; i++) {
        char *cand = path_join(p->include_paths[i], spec);
        if (file_exists(cand)) return cand;
        free(cand);
    }
    if (file_exists(spec)) return xstrdup(spec);
    return NULL;
}

/* -------------------- token-level macro expansion ----------------------- */
/*
 * Replace macro names in `line` (in-place into `out`). Object-like macros
 * get their body inlined. Function-like macros are expanded when followed
 * by a parenthesised argument list.
 *
 * This is a single-pass expander: we don't do iterative re-scanning beyond
 * one pass (good enough for LSL, where deep macro chains are rare).
 */
static void expand_line(Preproc *p, const char *line, Sbuf *out) {
    size_t i = 0; size_t n = strlen(line);
    int in_str = 0;
    while (i < n) {
        char c = line[i];
        if (in_str) {
            sb_putc(out, c);
            if (c == '\\' && i + 1 < n) { sb_putc(out, line[i+1]); i += 2; continue; }
            if (c == '"') in_str = 0;
            i++; continue;
        }
        if (c == '"') { sb_putc(out, c); in_str = 1; i++; continue; }
        if (c == '/' && i + 1 < n && line[i+1] == '/') {
            sb_puts(out, line + i);
            break;
        }
        if (is_id_start((unsigned char)c)) {
            size_t s = i;
            while (i < n && is_id_cont((unsigned char)line[i])) i++;
            size_t sz = i - s;
            char tmp[256];
            if (sz < sizeof tmp) {
                memcpy(tmp, line + s, sz); tmp[sz] = '\0';
                PpMacro *m = find_macro(p, tmp);
                if (m && !m->is_func) {
                    sb_puts(out, m->body);
                    continue;
                }
                if (m && m->is_func) {
                    size_t j = i;
                    while (j < n && (line[j] == ' ' || line[j] == '\t')) j++;
                    if (j < n && line[j] == '(') {
                        /* Collect args. */
                        j++;
                        int depth = 1;
                        char **args = NULL; int n_args = 0;
                        Sbuf cur; sb_init(&cur);
                        while (j < n && depth > 0) {
                            char d = line[j];
                            if (d == '(') { depth++; sb_putc(&cur, d); j++; }
                            else if (d == ')') {
                                depth--;
                                if (depth == 0) {
                                    args = (char**)xrealloc(args, sizeof(char*) * (n_args + 1));
                                    args[n_args++] = xstrdup(cur.buf ? cur.buf : "");
                                    free(cur.buf); sb_init(&cur);
                                    j++; break;
                                } else { sb_putc(&cur, d); j++; }
                            }
                            else if (d == ',' && depth == 1) {
                                args = (char**)xrealloc(args, sizeof(char*) * (n_args + 1));
                                args[n_args++] = xstrdup(cur.buf ? cur.buf : "");
                                free(cur.buf); sb_init(&cur);
                                j++;
                            }
                            else { sb_putc(&cur, d); j++; }
                        }
                        free(cur.buf);
                        if (n_args == m->n_params) {
                            /* Substitute params in body */
                            const char *b = m->body;
                            size_t bl = strlen(b); size_t k = 0;
                            while (k < bl) {
                                if (is_id_start((unsigned char)b[k])) {
                                    size_t ks = k;
                                    while (k < bl && is_id_cont((unsigned char)b[k])) k++;
                                    size_t kl = k - ks;
                                    char id[128];
                                    if (kl < sizeof id) {
                                        memcpy(id, b + ks, kl); id[kl] = '\0';
                                        int matched = -1;
                                        for (int q = 0; q < m->n_params; q++)
                                            if (strcmp(id, m->params[q]) == 0) { matched = q; break; }
                                        if (matched >= 0) sb_puts(out, args[matched]);
                                        else sb_putn(out, b + ks, kl);
                                    } else sb_putn(out, b + ks, kl);
                                } else { sb_putc(out, b[k++]); }
                            }
                            for (int q = 0; q < n_args; q++) free(args[q]);
                            free(args);
                            i = j;
                            continue;
                        } else {
                            SrcLoc loc = {0}; loc.line = 0; loc.col = 0;
                            diag_emit(p->diag, DIAG_ERROR, loc,
                                "macro '%s' takes %d argument%s, given %d",
                                m->name, m->n_params,
                                m->n_params == 1 ? "" : "s", n_args);
                            for (int q = 0; q < n_args; q++) free(args[q]);
                            free(args);
                            i = j;
                            continue;
                        }
                    }
                }
                /* No macro — emit as-is */
                sb_putn(out, line + s, sz);
                continue;
            }
            sb_putn(out, line + s, sz);
            continue;
        }
        sb_putc(out, c);
        i++;
    }
}

/* -------------------- directive parsing --------------------------------- */
static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}
static int parse_ident(const char **ps, char *out, size_t cap) {
    const char *s = skip_ws(*ps);
    if (!is_id_start((unsigned char)*s)) return 0;
    size_t n = 0;
    while (is_id_cont((unsigned char)*s) && n + 1 < cap) out[n++] = *s++;
    out[n] = '\0';
    *ps = s;
    return 1;
}

/* Conditional stack */
typedef struct {
    int active;      /* this branch is active */
    int matched;     /* some branch in this chain has been active */
    int parent_active;
} CondFrame;

typedef struct {
    CondFrame frames[32];
    int top;
} CondStack;

static int cond_active(CondStack *cs) {
    for (int i = 0; i < cs->top; i++) if (!cs->frames[i].active) return 0;
    return 1;
}

static void cond_push(CondStack *cs, int parent_active, int cond) {
    if (cs->top >= 31) return;
    cs->frames[cs->top].parent_active = parent_active;
    cs->frames[cs->top].active = parent_active ? cond : 0;
    cs->frames[cs->top].matched = parent_active ? cond : 1;
    cs->top++;
}
static void cond_else(CondStack *cs) {
    if (cs->top == 0) return;
    CondFrame *f = &cs->frames[cs->top - 1];
    f->active = f->parent_active && !f->matched;
    if (f->active) f->matched = 1;
}
static void cond_elif(CondStack *cs, int cond) {
    if (cs->top == 0) return;
    CondFrame *f = &cs->frames[cs->top - 1];
    if (f->matched) { f->active = 0; return; }
    f->active = f->parent_active && cond;
    if (f->active) f->matched = 1;
}
static void cond_pop(CondStack *cs) {
    if (cs->top > 0) cs->top--;
}

/* -------------------- main preprocessing loop --------------------------- */
typedef struct {
    Preproc *p;
    Sbuf out;
    CondStack cs;
    int depth;
} PpState;

static char *read_file_text(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    if (sz < 0) { fclose(f); return NULL; }
    char *b = (char*)xmalloc((size_t)sz + 1);
    size_t got = fread(b, 1, (size_t)sz, f);
    fclose(f);
    b[got] = '\0';
    if (len) *len = got;
    return b;
}

static void emit_line_marker(Sbuf *out, int line, const char *file) {
    sb_printf(out, "#line %d \"%s\"\n", line, file);
}

/* Evaluate `#if expr` and friends. We accept:
 *   defined(NAME)
 *   constants (integer literals)
 *   ! && ||
 *   parenthesised expressions
 * This is enough for the standard guard patterns most LSL devs need. */
typedef struct { const char *s; Preproc *p; } EvalCtx;
static long long eval_expr(EvalCtx *e);

static void eskip(EvalCtx *e) { while (*e->s == ' ' || *e->s == '\t') e->s++; }
static long long eval_atom(EvalCtx *e) {
    eskip(e);
    if (*e->s == '!') { e->s++; return !eval_atom(e); }
    if (*e->s == '(') { e->s++; long long v = eval_expr(e); eskip(e); if (*e->s == ')') e->s++; return v; }
    if (isdigit((unsigned char)*e->s)) { long long v = strtoll(e->s, (char**)&e->s, 0); return v; }
    if (is_id_start((unsigned char)*e->s)) {
        char id[128]; size_t n = 0;
        while (is_id_cont((unsigned char)*e->s) && n + 1 < sizeof id) id[n++] = *e->s++;
        id[n] = '\0';
        if (!strcmp(id, "defined")) {
            eskip(e);
            int paren = 0; if (*e->s == '(') { e->s++; paren = 1; eskip(e); }
            char name[128]; size_t m = 0;
            while (is_id_cont((unsigned char)*e->s) && m + 1 < sizeof name) name[m++] = *e->s++;
            name[m] = '\0';
            eskip(e);
            if (paren && *e->s == ')') e->s++;
            return find_macro(e->p, name) ? 1 : 0;
        }
        /* identifier — look up as macro and try integer interp */
        PpMacro *mc = find_macro(e->p, id);
        if (mc && !mc->is_func) return strtoll(mc->body, NULL, 0);
        return 0;
    }
    return 0;
}
static long long eval_and(EvalCtx *e) {
    long long v = eval_atom(e);
    eskip(e);
    while (e->s[0] == '&' && e->s[1] == '&') { e->s += 2; long long r = eval_atom(e); v = (v && r); eskip(e); }
    return v;
}
static long long eval_expr(EvalCtx *e) {
    long long v = eval_and(e);
    eskip(e);
    while (e->s[0] == '|' && e->s[1] == '|') { e->s += 2; long long r = eval_and(e); v = (v || r); eskip(e); }
    return v;
}

static int preprocess_file(PpState *st, const char *path) {
    if (st->depth >= st->p->max_include_depth) {
        SrcLoc l = {0}; l.line = 0; l.col = 0;
        diag_emit(st->p->diag, DIAG_ERROR, l,
            "include depth exceeded %d while opening '%s'", st->p->max_include_depth, path);
        return 0;
    }
    size_t len = 0;
    char *src = read_file_text(path, &len);
    if (!src) {
        SrcLoc l = {0}; l.line = 0; l.col = 0;
        diag_emit(st->p->diag, DIAG_ERROR, l,
            "cannot open file '%s': %s", path, strerror(errno));
        return 0;
    }
    char *base_dir = dirname_of(path);

    emit_line_marker(&st->out, 1, path);

    int line_no = 1;
    size_t i = 0;
    /* Process line-by-line */
    while (i <= len) {
        /* Read a line into a small heap buffer (handle long lines) */
        Sbuf line; sb_init(&line);
        while (i < len && src[i] != '\n') { sb_putc(&line, src[i++]); }
        const char *ln = line.buf ? line.buf : "";
        const char *p = skip_ws(ln);

        if (*p == '#') {
            const char *q = skip_ws(p + 1);
            char dir[32]; size_t dn = 0;
            while (is_id_cont((unsigned char)*q) && dn + 1 < sizeof dir) dir[dn++] = *q++;
            dir[dn] = '\0';
            const char *arg = skip_ws(q);

            int active = cond_active(&st->cs);

            if (!strcmp(dir, "include")) {
                if (active) {
                    /* parse "path" or <path> */
                    const char *start = NULL, *end = NULL;
                    int sys = 0;
                    if (*arg == '"') { start = arg + 1; end = strchr(start, '"'); }
                    else if (*arg == '<') { sys = 1; start = arg + 1; end = strchr(start, '>'); }
                    if (!start || !end) {
                        SrcLoc l = {0}; l.file = path; l.line = line_no;
                        diag_emit(st->p->diag, DIAG_ERROR, l,
                            "malformed #include directive (expected \"path\" or <path>)");
                    } else {
                        char *spec = xstrndup(start, (size_t)(end - start));
                        char *resolved = resolve_include(st->p, base_dir, spec, sys);
                        if (!resolved) {
                            SrcLoc l = {0}; l.file = path; l.line = line_no;
                            diag_emit(st->p->diag, DIAG_ERROR, l,
                                "cannot find include file '%s'", spec);
                            diag_hint(st->p->diag,
                                "search directories: %s%s",
                                sys ? "(only -I paths, system include)" : "(current dir then -I paths)",
                                st->p->n_include_paths ? "" : " — none configured");
                        } else {
                            st->depth++;
                            preprocess_file(st, resolved);
                            st->depth--;
                            emit_line_marker(&st->out, line_no + 1, path);
                            free(resolved);
                        }
                        free(spec);
                    }
                }
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "define")) {
                if (active) {
                    /* parse name (and optional param list) */
                    char name[128]; size_t nl = 0;
                    while (is_id_cont((unsigned char)*arg) && nl + 1 < sizeof name) name[nl++] = *arg++;
                    name[nl] = '\0';
                    int is_func = 0; int n_params = 0; char **params = NULL;
                    if (*arg == '(') {
                        is_func = 1; arg++;
                        for (;;) {
                            arg = skip_ws(arg);
                            if (*arg == ')') { arg++; break; }
                            char pn[128]; size_t pl = 0;
                            while (is_id_cont((unsigned char)*arg) && pl + 1 < sizeof pn) pn[pl++] = *arg++;
                            pn[pl] = '\0';
                            if (!pl) break;
                            params = (char**)xrealloc(params, sizeof(char*) * (n_params + 1));
                            params[n_params++] = xstrdup(pn);
                            arg = skip_ws(arg);
                            if (*arg == ',') arg++;
                        }
                    }
                    arg = skip_ws(arg);
                    add_macro(st->p, name, is_func, n_params, params, arg);
                }
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "undef")) {
                if (active) {
                    char name[128]; size_t nl = 0;
                    while (is_id_cont((unsigned char)*arg) && nl + 1 < sizeof name) name[nl++] = *arg++;
                    name[nl] = '\0';
                    PpMacro **pp = &st->p->macros;
                    while (*pp) {
                        if (strcmp((*pp)->name, name) == 0) {
                            PpMacro *gone = *pp; *pp = gone->next;
                            free(gone->name); free(gone->body);
                            for (int k = 0; k < gone->n_params; k++) free(gone->params[k]);
                            free(gone->params); free(gone);
                            break;
                        }
                        pp = &(*pp)->next;
                    }
                }
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "ifdef")) {
                char name[128]; size_t nl = 0;
                while (is_id_cont((unsigned char)*arg) && nl + 1 < sizeof name) name[nl++] = *arg++;
                name[nl] = '\0';
                cond_push(&st->cs, active, find_macro(st->p, name) ? 1 : 0);
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "ifndef")) {
                char name[128]; size_t nl = 0;
                while (is_id_cont((unsigned char)*arg) && nl + 1 < sizeof name) name[nl++] = *arg++;
                name[nl] = '\0';
                cond_push(&st->cs, active, find_macro(st->p, name) ? 0 : 1);
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "if")) {
                EvalCtx e; e.s = arg; e.p = st->p;
                long long v = eval_expr(&e);
                cond_push(&st->cs, active, v ? 1 : 0);
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "elif")) {
                EvalCtx e; e.s = arg; e.p = st->p;
                long long v = eval_expr(&e);
                cond_elif(&st->cs, v ? 1 : 0);
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "else")) {
                cond_else(&st->cs);
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "endif")) {
                cond_pop(&st->cs);
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "error")) {
                if (active) {
                    SrcLoc l = {0}; l.file = path; l.line = line_no;
                    diag_emit(st->p->diag, DIAG_ERROR, l, "#error: %s", arg);
                }
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "warning")) {
                if (active) {
                    SrcLoc l = {0}; l.file = path; l.line = line_no;
                    diag_emit(st->p->diag, DIAG_WARNING, l, "#warning: %s", arg);
                }
                sb_putc(&st->out, '\n');
            } else if (!strcmp(dir, "pragma") || !strcmp(dir, "line")) {
                /* Pass-through */
                sb_putc(&st->out, '#');
                sb_puts(&st->out, p + 1);
                sb_putc(&st->out, '\n');
            } else {
                if (active) {
                    SrcLoc l = {0}; l.file = path; l.line = line_no;
                    diag_emit(st->p->diag, DIAG_WARNING, l,
                        "unknown preprocessor directive #%s — ignoring", dir);
                }
                sb_putc(&st->out, '\n');
            }
        } else {
            /* Plain (or comment) line. Expand macros if active. */
            if (cond_active(&st->cs)) {
                expand_line(st->p, ln, &st->out);
            }
            sb_putc(&st->out, '\n');
        }

        free(line.buf);
        line_no++;
        if (i >= len) break;
        i++;  /* consume the '\n' */
    }
    free(src);
    free(base_dir);
    return 1;
}

char *preproc_run(Preproc *p, const char *path, size_t *out_len) {
    PpState st;
    memset(&st, 0, sizeof st);
    st.p = p;
    sb_init(&st.out);
    if (!preprocess_file(&st, path)) {
        free(st.out.buf);
        return NULL;
    }
    if (out_len) *out_len = st.out.len;
    return st.out.buf ? st.out.buf : xstrdup("");
}
