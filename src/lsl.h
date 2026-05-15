/* lsl.h - shared header for lslc (LSL Mono compiler) */
#ifndef LSL_H
#define LSL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* ----------------------------- Diagnostics ------------------------------ */

typedef enum {
    DIAG_NOTE,
    DIAG_HINT,
    DIAG_WARNING,
    DIAG_ERROR,
    DIAG_FATAL
} DiagKind;

typedef struct {
    const char *file;
    int line;     /* 1-based */
    int col;      /* 1-based */
    size_t off;   /* offset into source */
} SrcLoc;

/* A source-range. end_off is exclusive. */
typedef struct {
    SrcLoc start;
    size_t end_off;
} SrcSpan;

/* Target VM. Used to gate Mono-only builtins and to choose a memory
 * budget. Default is Mono. */
typedef enum { LSL_MONO = 0, LSL_LSO = 1 } LslTarget;

typedef struct DiagCtx {
    const char *file;
    const char *src;     /* full source buffer for caret display */
    size_t src_len;
    int errors;
    int warnings;
    int werror;          /* treat warnings as errors */
    int wall;            /* enable extra warnings */
    int max_errors;
    FILE *out;           /* stderr usually */
    int color;
    int no_caret;
    LslTarget target;    /* LSL_MONO (default) / LSL_LSO */
    size_t memory_budget;/* per-script memory budget in bytes */
} DiagCtx;

void diag_init(DiagCtx *d, const char *file, const char *src, size_t src_len);

/* Emit a diagnostic at a single point. */
void diag_emit(DiagCtx *d, DiagKind kind, SrcLoc loc, const char *fmt, ...);

/* Emit a diagnostic that highlights a range [start_off, end_off). */
void diag_emit_range(DiagCtx *d, DiagKind kind, SrcLoc loc, size_t end_off,
                     const char *fmt, ...);

/* Emit a note/hint attached visually to the previous diagnostic (just a note). */
void diag_note(DiagCtx *d, SrcLoc loc, const char *fmt, ...);
void diag_hint(DiagCtx *d, const char *fmt, ...);

/* Suggest the closest matching identifier in a list. Returns the chosen
 * name (pointer to one of the items) or NULL. */
const char *diag_did_you_mean(const char *needle,
                              const char *const *candidates, int n,
                              int max_distance);

int  diag_should_stop(const DiagCtx *d);

/* ------------------------------- Types ---------------------------------- */

typedef enum {
    T_VOID = 0,      /* used for procedures (no return) */
    T_INTEGER,
    T_FLOAT,
    T_STRING,
    T_KEY,
    T_VECTOR,
    T_ROTATION,
    T_LIST,
    T_ANY,           /* internal: used for varargs / generic placeholders */
    T_ERROR          /* error sentinel used to suppress cascading errors */
} TypeKind;

const char *type_name(TypeKind t);
TypeKind type_from_keyword(const char *s);
int type_is_numeric(TypeKind t);
int type_implicit_assignable(TypeKind to, TypeKind from);
int type_explicit_castable(TypeKind to, TypeKind from);
TypeKind type_binop_result(int op, TypeKind l, TypeKind r);

/* ------------------------------- Tokens --------------------------------- */

typedef enum {
    TK_EOF = 0,
    TK_IDENT,
    TK_INT,          /* integer literal */
    TK_FLOAT,        /* float literal */
    TK_STRING,       /* string literal (already de-escaped) */

    /* keywords */
    TK_KW_INTEGER, TK_KW_FLOAT, TK_KW_STRING, TK_KW_KEY, TK_KW_VECTOR,
    TK_KW_ROTATION, TK_KW_QUATERNION, TK_KW_LIST,
    TK_KW_DEFAULT, TK_KW_STATE, TK_KW_IF, TK_KW_ELSE, TK_KW_WHILE,
    TK_KW_DO, TK_KW_FOR, TK_KW_RETURN, TK_KW_JUMP, TK_KW_PRINT,

    /* punctuation / operators */
    TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE, TK_LBRACK, TK_RBRACK,
    TK_LANGLE, TK_RANGLE,
    TK_SEMI, TK_COMMA, TK_AT, TK_COLON, TK_DOT,
    TK_ASSIGN, TK_PLUS_ASSIGN, TK_MINUS_ASSIGN, TK_STAR_ASSIGN,
    TK_SLASH_ASSIGN, TK_PERCENT_ASSIGN,
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
    TK_AND, TK_OR, TK_XOR, TK_TILDE,
    TK_LAND, TK_LOR, TK_NOT,
    TK_EQ, TK_NEQ, TK_LE, TK_GE, TK_LT, TK_GT,
    TK_SHL, TK_SHR,
    TK_INC, TK_DEC
} TokKind;

typedef struct {
    TokKind kind;
    SrcLoc  loc;
    size_t  end_off;   /* exclusive end offset */
    /* lexeme value */
    union {
        long long i;
        double f;
        char *s;       /* identifier name OR decoded string literal */
    } v;
} Token;

/* ------------------------------- Lexer ---------------------------------- */

typedef struct {
    const char *src;
    size_t len;
    size_t off;
    int line;
    int col;
    const char *file;
    DiagCtx *diag;
} Lexer;

void lexer_init(Lexer *L, const char *file, const char *src, size_t len, DiagCtx *d);
Token lexer_next(Lexer *L);
void  token_free(Token *t);
const char *tok_kind_name(TokKind k);
const char *tok_kind_display(TokKind k);

/* --------------------------------- AST ---------------------------------- */

typedef struct Expr   Expr;
typedef struct Stmt   Stmt;
typedef struct Param  Param;
typedef struct GlobalVar GlobalVar;
typedef struct FuncDecl  FuncDecl;
typedef struct StateDecl StateDecl;
typedef struct EventDecl EventDecl;
typedef struct ScriptAST ScriptAST;

typedef enum {
    E_INT_LIT,
    E_FLOAT_LIT,
    E_STRING_LIT,
    E_IDENT,
    E_VECTOR_LIT,      /* < x, y, z > */
    E_ROT_LIT,         /* < x, y, z, w > */
    E_LIST_LIT,        /* [ ... ] */
    E_CALL,            /* f(args) */
    E_MEMBER,          /* v.x v.y v.z v.s (member access) */
    E_CAST,            /* (T) expr */
    E_UNARY,           /* + - ! ~ ++x --x */
    E_POSTFIX,         /* x++ x-- */
    E_BINARY,          /* arithmetic, comparison, bitwise, logical */
    E_ASSIGN           /* = += -= *= /= %= */
} ExprKind;

struct Expr {
    ExprKind kind;
    SrcLoc loc;
    size_t end_off;
    TypeKind type;       /* filled by type checker */
    int is_lvalue;       /* filled by type checker */

    union {
        long long ilit;
        double flit;
        char *slit;                 /* heap, decoded */
        char *ident;                /* heap */
        struct { Expr *x,*y,*z,*w; } vec;
        struct { Expr **items; int n; } list;
        struct { char *name; Expr **args; int n_args; } call;
        struct { Expr *base; char member; } memb;
        struct { TypeKind to; Expr *inner; } cast;
        struct { int op; Expr *inner; } un;
        struct { int op; Expr *inner; } post;
        struct { int op; Expr *l, *r; } bin;
        struct { int op; Expr *l, *r; } asn;
    } u;
};

typedef enum {
    S_EXPR,
    S_DECL,        /* local var decl */
    S_BLOCK,
    S_IF,
    S_WHILE,
    S_DO,
    S_FOR,
    S_RETURN,
    S_JUMP,
    S_LABEL,
    S_STATECHG,
    S_EMPTY
} StmtKind;

struct Stmt {
    StmtKind kind;
    SrcLoc loc;
    union {
        Expr *expr;                                    /* S_EXPR */
        struct { TypeKind t; char *name; Expr *init; SrcLoc name_loc; } decl;
        struct { Stmt **stmts; int n; } block;
        struct { Expr *cond; Stmt *then_s; Stmt *else_s; } if_s;
        struct { Expr *cond; Stmt *body; } while_s;
        struct { Stmt *body; Expr *cond; } do_s;
        struct {
            Expr **init;  int n_init;
            Expr  *cond;
            Expr **post;  int n_post;
            Stmt  *body;
        } for_s;
        struct { Expr *expr; int has_value; } ret;
        struct { char *label; SrcLoc lbl_loc; } jmp;
        struct { char *label; } lbl;
        struct { char *state; SrcLoc name_loc; } stc;
    } u;
};

struct Param {
    TypeKind type;
    char *name;
    SrcLoc loc;
};

struct GlobalVar {
    TypeKind type;
    char *name;
    Expr *init;                /* may be NULL */
    SrcLoc loc;
};

struct FuncDecl {
    TypeKind ret;              /* T_VOID if no declared return type */
    int has_return_type;       /* whether user wrote a return type */
    char *name;
    Param *params;
    int n_params;
    Stmt *body;                /* block */
    SrcLoc loc;
};

struct EventDecl {
    char *name;
    Param *params;
    int n_params;
    Stmt *body;
    SrcLoc loc;
};

struct StateDecl {
    char *name;
    int is_default;
    EventDecl *events;
    int n_events;
    SrcLoc loc;
};

struct ScriptAST {
    GlobalVar *globals;  int n_globals;
    FuncDecl  *funcs;    int n_funcs;
    StateDecl *states;   int n_states;
    const char *file;
};

void ast_free_expr(Expr *e);
void ast_free_stmt(Stmt *s);
void ast_free_script(ScriptAST *s);

/* ------------------------------ Parser ---------------------------------- */

typedef struct {
    Lexer *lex;
    DiagCtx *diag;
    Token cur, peek;
    int has_peek;
    int panic;
} Parser;

void parser_init(Parser *p, Lexer *lex, DiagCtx *diag);
int  parser_parse_script(Parser *p, ScriptAST *out);

/* ----------------------------- Built-ins -------------------------------- */

typedef struct {
    const char *name;
    TypeKind ret;
    int n_params;
    TypeKind params[12];
    unsigned int flags;
} BuiltinFn;

#define BFN_DEPRECATED 0x1
#define BFN_GODLY      0x2
#define BFN_EXPERIENCE 0x4
#define BFN_MONO_ONLY  0x8   /* unavailable under LSO */

typedef struct {
    const char *name;
    TypeKind type;
    long long  ival;
    double     fval;
    const char *sval;
    int has_value;
} BuiltinConst;

typedef struct {
    const char *name;
    int n_params;
    TypeKind params[8];
    const char *param_names[8];
} BuiltinEvent;

const BuiltinFn   *bi_lookup_fn(const char *name);
const BuiltinConst*bi_lookup_const(const char *name);
const BuiltinEvent*bi_lookup_event(const char *name);

extern const BuiltinFn    BI_FN[];      extern const int BI_FN_N;
extern const BuiltinConst BI_CONST[];   extern const int BI_CONST_N;
extern const BuiltinEvent BI_EVENT[];   extern const int BI_EVENT_N;

/* ----------------------------- Symbol table ----------------------------- */

typedef enum {
    SYM_GLOBAL,
    SYM_LOCAL,
    SYM_PARAM,
    SYM_FUNC,
    SYM_STATE,
    SYM_LABEL,
    SYM_BUILTIN_FN,
    SYM_BUILTIN_CONST
} SymKind;

typedef struct Sym {
    char *name;
    SymKind kind;
    TypeKind type;
    int scope_depth;
    SrcLoc loc;
    int n_params;
    TypeKind params[16];
    struct Sym *next_in_scope;
    struct Sym *next_in_bucket;
    int used;
} Sym;

typedef struct Scope {
    struct Scope *parent;
    Sym **buckets;
    int nbuckets;
    Sym *list;
    int depth;
} Scope;

typedef struct {
    Scope *cur;
    Scope *global;
    DiagCtx *diag;
} SymTab;

void sym_init(SymTab *s, DiagCtx *d);
void sym_free(SymTab *s);
void sym_push_scope(SymTab *s);
void sym_pop_scope(SymTab *s);
Sym *sym_define(SymTab *s, const char *name, SymKind k, TypeKind t, SrcLoc loc);
Sym *sym_lookup(SymTab *s, const char *name);
Sym *sym_lookup_local(SymTab *s, const char *name);
Sym *sym_lookup_global(SymTab *s, const char *name);

/* ----------------------------- Semantic --------------------------------- */

int sem_check(ScriptAST *ast, DiagCtx *d);

/* ----------------------------- Folding ---------------------------------- */
/*
 * fold_expr walks an expression bottom-up and, where every operand is a
 * compile-time constant, replaces the subtree with the folded literal.
 * Returns 1 if the resulting expression is a constant literal.
 *
 * Built-in constants (TRUE, FALSE, PI, …) count as constants.
 * Variable references and function calls are NOT constants.
 */
int fold_expr(Expr **pe, DiagCtx *d);
int expr_is_constant(Expr *e);

/* ----------------------------- Preprocessor ----------------------------- */

typedef struct PpMacro {
    char *name;
    int   is_func;          /* function-like */
    int   n_params;
    char **params;
    char *body;
    struct PpMacro *next;
} PpMacro;

typedef struct Preproc {
    PpMacro *macros;
    char **include_paths;   /* search paths, last = NULL */
    int n_include_paths;
    DiagCtx *diag;
    int max_include_depth;
} Preproc;

void preproc_init(Preproc *p, DiagCtx *d);
void preproc_free(Preproc *p);
void preproc_add_include_path(Preproc *p, const char *path);
void preproc_define_cli(Preproc *p, const char *spec);  /* "NAME" or "NAME=val" */

/* Read+preprocess a file. Returns a newly-allocated buffer containing
 * preprocessed text with embedded `#line N "file"` markers. *out_len is
 * set. Returns NULL on hard I/O failure. */
char *preproc_run(Preproc *p, const char *path, size_t *out_len);

/* ----------------------------- Bytecode --------------------------------- */
/*
 * After semantic analysis succeeds, write the typed AST out as a compact
 * binary file (the SLBC format). Returns non-zero on success. Used by
 * the slemu runtime as input. See emit.c for the file layout.
 */
int emit_bytecode(ScriptAST *ast, const char *out_path, int lso, DiagCtx *diag);

/* --------------------------------- Util --------------------------------- */

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t s);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);
char *read_file_all(const char *path, size_t *len_out);
int   levenshtein(const char *a, const char *b, int cutoff);

#endif /* LSL_H */
