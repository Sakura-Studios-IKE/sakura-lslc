/* main.c - lslc compiler driver */
#include "lsl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LSLC_VERSION
#define LSLC_VERSION "1.0.0"
#endif

static void show_help(const char *prog) {
    fprintf(stdout,
"Usage: %s [options] file.lsl [more.lsl ...]\n"
"\n"
"lslc - LSL Mono / LSO syntax & type checker. Validates Second Life and\n"
"       OpenSim LSL scripts the way the official compiler does, but with\n"
"       much richer diagnostics and from the command line.\n"
"\n"
"Targeting:\n"
"  --mono                   Target the Mono VM (default; 64 KiB budget)\n"
"  --lso                    Target the legacy LSO VM (16 KiB budget, rejects\n"
"                           Mono-only built-ins such as llJson*, llLinksetData*,\n"
"                           llCastRay, llChar, llOrd, llHMAC, llTransferLindenDollars)\n"
"  --memory-budget=N        Override the memory budget in bytes\n"
"  --no-memory-check        Disable source-size budget warnings/errors\n"
"\n"
"Diagnostics:\n"
"  -W, --wall               Enable extra warnings\n"
"  -Werror                  Treat warnings as errors\n"
"  -fsyntax-only            (default) Only check syntax and types\n"
"  -fno-color               Disable ANSI color in diagnostics\n"
"  -fmax-errors=N           Stop after N errors (default: 50)\n"
"  -q, --quiet              Print only summary on errors, suppress source carets\n"
"\n"
"Preprocessor:\n"
"  -I PATH                  Add a directory to the #include search path\n"
"  -D NAME[=value]          Pre-define a macro (like cpp's -D)\n"
"  -E                       Print the preprocessed source and exit\n"
"  --no-preprocess          Skip the preprocessor entirely\n"
"\n"
"Other:\n"
"  -o, --output FILE        Output path for diagnostics (default: stderr)\n"
"  --list-builtins          Print all built-in function/constant/event names\n"
"      --version            Print version and exit\n"
"  -h, --help               Show this help message\n"
"\n"
"Exit codes:\n"
"   0  success (zero errors)\n"
"   1  one or more files failed to compile\n"
"   2  internal error (out of memory, cannot open file, etc.)\n",
        prog);
}

static void list_builtins(void) {
    printf("# %d functions\n", BI_FN_N);
    for (int i = 0; i < BI_FN_N; i++) printf("%s\n", BI_FN[i].name);
    printf("# %d constants\n", BI_CONST_N);
    for (int i = 0; i < BI_CONST_N; i++) printf("%s\n", BI_CONST[i].name);
    printf("# %d events\n", BI_EVENT_N);
    for (int i = 0; i < BI_EVENT_N; i++) printf("%s\n", BI_EVENT[i].name);
}

typedef struct {
    int wall, werror, quiet, color, max_errors;
    LslTarget target;
    long memory_budget;     /* -1 = derive from target, 0 = disable */
    int dump_preproc;       /* -E */
    int no_preproc;         /* --no-preprocess */
    int n_include_paths;
    char **include_paths;
    int n_defines;
    char **defines;
} CliOpts;

/* Rough script-bytecode estimate: source size minus comments and
 * whitespace. LSL Mono compiles globals + code + string-table into
 * an in-region heap that is similar in magnitude to the trimmed
 * source size, though string literals and large list initialisers
 * can dominate. This is a heuristic budget check, not an exact
 * bytecode size — but it's the same proxy the SL viewer's
 * "estimated bytecode size" widget uses. */
static size_t estimate_bytecode_size(const char *src, size_t len) {
    size_t i = 0, n = 0;
    int in_str = 0;
    while (i < len) {
        char c = src[i];
        if (!in_str && c == '/' && i + 1 < len && src[i+1] == '/') {
            while (i < len && src[i] != '\n') i++;
            continue;
        }
        if (!in_str && c == '/' && i + 1 < len && src[i+1] == '*') {
            i += 2;
            while (i + 1 < len && !(src[i] == '*' && src[i+1] == '/')) i++;
            if (i + 1 < len) i += 2;
            continue;
        }
        if (c == '"') { in_str = !in_str; n++; i++; continue; }
        if (in_str) { n++; i++; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { i++; continue; }
        n++; i++;
    }
    return n;
}

static int compile_file(const char *path, const CliOpts *opts) {
    size_t len = 0;
    char *src = NULL;

    DiagCtx d_pp;
    diag_init(&d_pp, path, NULL, 0);
    d_pp.wall = opts->wall; d_pp.werror = opts->werror;
    if (!opts->color) d_pp.color = 0;
    if (opts->quiet) d_pp.no_caret = 1;

    if (opts->no_preproc) {
        src = read_file_all(path, &len);
        if (!src) return 2;
    } else {
        Preproc pp; preproc_init(&pp, &d_pp);
        for (int k = 0; k < opts->n_include_paths; k++)
            preproc_add_include_path(&pp, opts->include_paths[k]);
        for (int k = 0; k < opts->n_defines; k++)
            preproc_define_cli(&pp, opts->defines[k]);
        src = preproc_run(&pp, path, &len);
        preproc_free(&pp);
        if (!src) return 2;
        if (d_pp.errors > 0) {
            fprintf(stderr, "%s: %d preprocessor error%s\n",
                path, d_pp.errors, d_pp.errors == 1 ? "" : "s");
            free(src);
            return 1;
        }
        if (opts->dump_preproc) {
            fwrite(src, 1, len, stdout);
            free(src);
            return 0;
        }
    }

    DiagCtx d; diag_init(&d, path, src, len);
    d.wall = opts->wall; d.werror = opts->werror;
    if (!opts->color) d.color = 0;
    if (opts->quiet) d.no_caret = 1;
    if (opts->max_errors > 0) d.max_errors = opts->max_errors;
    d.target = opts->target;
    if (opts->target == LSL_LSO) d.memory_budget = 16384;
    if (opts->memory_budget >= 0) d.memory_budget = (size_t)opts->memory_budget;

    /* ---- Memory budget warning, based on a source-size proxy ---- */
    if (d.memory_budget > 0) {
        size_t est = estimate_bytecode_size(src, len);
        size_t budget = d.memory_budget;
        const char *tname = (d.target == LSL_LSO) ? "LSO" : "Mono";
        SrcLoc top = {0}; top.file = path; top.line = 1; top.col = 1;
        if (est > budget) {
            diag_emit(&d, DIAG_ERROR, top,
                "script source is ~%zu bytes after stripping comments/whitespace, "
                "which exceeds the LSL %s per-script budget of %zu bytes",
                est, tname, budget);
            diag_hint(&d, "the official compiler will reject this script at upload time; split it into smaller scripts or move data to a notecard");
        } else if (est * 100 / budget >= 80) {
            diag_emit(&d, DIAG_WARNING, top,
                "script source is ~%zu bytes (%zu%% of the LSL %s budget of %zu bytes)",
                est, est * 100 / budget, tname, budget);
            diag_hint(&d, "you are close to the limit; consider trimming string literals or splitting the script");
        }
    }

    Lexer L; lexer_init(&L, path, src, len, &d);
    Parser P; parser_init(&P, &L, &d);
    ScriptAST ast;
    parser_parse_script(&P, &ast);

    if (d.errors == 0) {
        sem_check(&ast, &d);
    } else {
        /* still run sem_check to catch what we can — but only if not too noisy */
        if (d.errors < 5) sem_check(&ast, &d);
    }

    int rc;
    if (d.errors > 0) {
        fprintf(stderr,
                "%s: %d error%s, %d warning%s\n",
                path, d.errors, d.errors == 1 ? "" : "s",
                d.warnings, d.warnings == 1 ? "" : "s");
        rc = 1;
    } else {
        if (d.warnings > 0 && !opts->quiet) {
            fprintf(stderr,
                "%s: OK (%d warning%s)\n", path,
                d.warnings, d.warnings == 1 ? "" : "s");
        } else if (!opts->quiet) {
            fprintf(stderr, "%s: OK\n", path);
        }
        rc = 0;
    }

    ast_free_script(&ast);
    /* The current/peek tokens may still hold heap copies */
    token_free(&P.cur);
    token_free(&P.peek);
    free(src);
    return rc;
}

int main(int argc, char **argv) {
    CliOpts opts;
    memset(&opts, 0, sizeof opts);
    opts.color = 1;
    opts.target = LSL_MONO;
    opts.memory_budget = -1;       /* -1 = derive from target */
    int rc_overall = 0;

    int i = 1;
    while (i < argc) {
        const char *a = argv[i];
        if (a[0] != '-') break;
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { show_help(argv[0]); return 0; }
        if (!strcmp(a, "--version")) { printf("lslc %s\n", LSLC_VERSION); return 0; }
        if (!strcmp(a, "--list-builtins")) { list_builtins(); return 0; }
        if (!strcmp(a, "-W") || !strcmp(a, "--wall") || !strcmp(a, "-Wall")) { opts.wall = 1; i++; continue; }
        if (!strcmp(a, "-Werror")) { opts.werror = 1; i++; continue; }
        if (!strcmp(a, "-fsyntax-only")) { i++; continue; }
        if (!strcmp(a, "-fno-color")) { opts.color = 0; i++; continue; }
        if (!strcmp(a, "-q") || !strcmp(a, "--quiet")) { opts.quiet = 1; i++; continue; }
        if (!strncmp(a, "-fmax-errors=", 13)) { opts.max_errors = atoi(a + 13); i++; continue; }
        if (!strcmp(a, "--mono")) { opts.target = LSL_MONO; i++; continue; }
        if (!strcmp(a, "--lso"))  { opts.target = LSL_LSO;  i++; continue; }
        if (!strncmp(a, "--memory-budget=", 16)) {
            opts.memory_budget = atol(a + 16);
            if (opts.memory_budget < 0) opts.memory_budget = 0;
            i++; continue;
        }
        if (!strcmp(a, "--no-memory-check")) { opts.memory_budget = 0; i++; continue; }
        if (!strcmp(a, "-E")) { opts.dump_preproc = 1; i++; continue; }
        if (!strcmp(a, "--no-preprocess")) { opts.no_preproc = 1; i++; continue; }
        if (!strncmp(a, "-I", 2)) {
            const char *val = (a[2] != '\0') ? a + 2 : (i + 1 < argc ? argv[++i] : NULL);
            if (!val) { fprintf(stderr, "lslc: -I needs a path\n"); return 2; }
            opts.include_paths = (char**)xrealloc(opts.include_paths, sizeof(char*) * (opts.n_include_paths + 1));
            opts.include_paths[opts.n_include_paths++] = xstrdup(val);
            i++; continue;
        }
        if (!strncmp(a, "-D", 2)) {
            const char *val = (a[2] != '\0') ? a + 2 : (i + 1 < argc ? argv[++i] : NULL);
            if (!val) { fprintf(stderr, "lslc: -D needs NAME[=value]\n"); return 2; }
            opts.defines = (char**)xrealloc(opts.defines, sizeof(char*) * (opts.n_defines + 1));
            opts.defines[opts.n_defines++] = xstrdup(val);
            i++; continue;
        }
        if (!strcmp(a, "-o") || !strcmp(a, "--output")) {
            if (i + 1 >= argc) { fprintf(stderr, "lslc: option %s requires a path\n", a); return 2; }
            if (!freopen(argv[i+1], "w", stderr)) {
                fprintf(stdout, "lslc: cannot write to %s\n", argv[i+1]); return 2;
            }
            i += 2; continue;
        }
        if (!strcmp(a, "--")) { i++; break; }
        fprintf(stderr, "lslc: unrecognized option '%s' (use --help)\n", a);
        return 2;
    }
    if (i >= argc) {
        fprintf(stderr, "lslc: no input files\n");
        fprintf(stderr, "use --help for usage\n");
        return 2;
    }
    for (; i < argc; i++) {
        int rc = compile_file(argv[i], &opts);
        if (rc != 0) rc_overall = rc;
    }
    return rc_overall;
}
