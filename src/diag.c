/* diag.c - rich gcc-style diagnostics with ranges, notes and suggestions */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include "lsl.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

void diag_init(DiagCtx *d, const char *file, const char *src, size_t src_len) {
    memset(d, 0, sizeof *d);
    d->file = file;
    d->src = src;
    d->src_len = src_len;
    d->out = stderr;
    d->max_errors = 50;
    d->color = ISATTY(FILENO(stderr));
    d->target = LSL_MONO;
    d->memory_budget = 65536;    /* Mono default: 64 KB per script */
}

static const char *kind_label(DiagKind k) {
    switch (k) {
        case DIAG_NOTE:    return "note";
        case DIAG_HINT:    return "help";
        case DIAG_WARNING: return "warning";
        case DIAG_ERROR:   return "error";
        case DIAG_FATAL:   return "fatal error";
    }
    return "?";
}

static const char *kind_color(DiagKind k) {
    switch (k) {
        case DIAG_NOTE:    return "\x1b[36;1m";
        case DIAG_HINT:    return "\x1b[36;1m";
        case DIAG_WARNING: return "\x1b[35;1m";
        case DIAG_ERROR:   return "\x1b[31;1m";
        case DIAG_FATAL:   return "\x1b[31;1m";
    }
    return "";
}

static int digits10(int n) {
    if (n < 0) n = -n;
    int d = 1; while (n >= 10) { n /= 10; d++; } return d;
}

/* Print one or more source lines covering [start_off, end_off) with carets
 * pointing at the range. Handles single- and multi-line ranges. */
static void print_caret(DiagCtx *d, SrcLoc loc, size_t end_off) {
    if (!d->src || loc.off > d->src_len || d->no_caret) return;

    /* Find first line start */
    size_t s = loc.off;
    while (s > 0 && d->src[s-1] != '\n') s--;

    /* Find last line end (cover [loc.off, end_off)). */
    if (end_off <= loc.off) end_off = loc.off + 1;
    if (end_off > d->src_len) end_off = d->src_len;

    /* Walk lines from s, printing each one that intersects the range. */
    int cur_line = loc.line;
    size_t line_start = s;
    int gutter = digits10(cur_line + 4);
    if (gutter < 4) gutter = 4;

    while (line_start <= d->src_len) {
        size_t line_end = line_start;
        while (line_end < d->src_len && d->src[line_end] != '\n') line_end++;

        /* If this line is entirely past the range, stop. */
        if (line_start >= end_off) break;

        /* Print "   42 | source line" */
        if (d->color) fputs("\x1b[34m", d->out);
        fprintf(d->out, "  %*d | ", gutter, cur_line);
        if (d->color) fputs("\x1b[0m", d->out);
        fwrite(d->src + line_start, 1, line_end - line_start, d->out);
        fputc('\n', d->out);

        /* Print caret row */
        if (d->color) fputs("\x1b[34m", d->out);
        fprintf(d->out, "  %*s | ", gutter, "");
        if (d->color) fputs("\x1b[32;1m", d->out);

        size_t a = line_start, b = line_end;
        if (a < loc.off) a = loc.off;
        if (b > end_off) b = end_off;
        if (a < b) {
            /* Pad up to a with spaces (preserving tabs) */
            for (size_t i = line_start; i < a; i++)
                fputc(d->src[i] == '\t' ? '\t' : ' ', d->out);
            /* Caret + tildes for the highlighted span. */
            fputc('^', d->out);
            for (size_t i = a + 1; i < b; i++) fputc('~', d->out);
        }
        if (d->color) fputs("\x1b[0m", d->out);
        fputc('\n', d->out);

        if (line_end >= d->src_len) break;
        line_start = line_end + 1;
        cur_line++;
    }
}

static DiagKind g_last_kind = DIAG_NOTE;

static void emit_v(DiagCtx *d, DiagKind kind, SrcLoc loc, size_t end_off,
                   const char *fmt, va_list ap) {
    if (kind == DIAG_WARNING && d->werror) kind = DIAG_ERROR;

    const char *file = loc.file ? loc.file : d->file;
    if (!file) file = "<input>";

    if (d->color) fputs("\x1b[1m", d->out);
    fprintf(d->out, "%s:%d:%d: ", file, loc.line, loc.col);
    if (d->color) fputs("\x1b[0m", d->out);

    if (d->color) fputs(kind_color(kind), d->out);
    fprintf(d->out, "%s: ", kind_label(kind));
    if (d->color) fputs("\x1b[0m", d->out);

    vfprintf(d->out, fmt, ap);
    fputc('\n', d->out);

    if (loc.off <= d->src_len) print_caret(d, loc, end_off);

    if (kind == DIAG_ERROR || kind == DIAG_FATAL) d->errors++;
    else if (kind == DIAG_WARNING) d->warnings++;
    g_last_kind = kind;
}

void diag_emit(DiagCtx *d, DiagKind kind, SrcLoc loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit_v(d, kind, loc, loc.off + 1, fmt, ap);
    va_end(ap);
}

void diag_emit_range(DiagCtx *d, DiagKind kind, SrcLoc loc, size_t end_off,
                     const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit_v(d, kind, loc, end_off, fmt, ap);
    va_end(ap);
}

void diag_note(DiagCtx *d, SrcLoc loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit_v(d, DIAG_NOTE, loc, loc.off + 1, fmt, ap);
    va_end(ap);
}

void diag_hint(DiagCtx *d, const char *fmt, ...) {
    if (d->color) fputs("\x1b[36;1m", d->out);
    fputs("  help: ", d->out);
    if (d->color) fputs("\x1b[0m", d->out);
    va_list ap; va_start(ap, fmt);
    vfprintf(d->out, fmt, ap);
    va_end(ap);
    fputc('\n', d->out);
}

const char *diag_did_you_mean(const char *needle,
                              const char *const *candidates, int n,
                              int max_distance) {
    if (!needle || !*needle || n <= 0) return NULL;
    int best = max_distance + 1;
    const char *best_name = NULL;
    int needle_len = (int)strlen(needle);
    if (needle_len <= 2 && max_distance > 1) max_distance = 1;
    for (int i = 0; i < n; i++) {
        if (!candidates[i]) continue;
        int d = levenshtein(needle, candidates[i], max_distance);
        if (d < best) { best = d; best_name = candidates[i]; }
    }
    if (best > max_distance) return NULL;
    return best_name;
}

int diag_should_stop(const DiagCtx *d) {
    if (d->errors >= d->max_errors) return 1;
    return 0;
}
