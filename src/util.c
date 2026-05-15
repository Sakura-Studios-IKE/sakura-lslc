/* util.c - memory, file and string helpers */
#include "lsl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void oom(void) {
    fprintf(stderr, "lslc: out of memory\n");
    exit(2);
}

void *xmalloc(size_t n) {
    if (n == 0) n = 1;
    void *p = malloc(n);
    if (!p) oom();
    return p;
}

void *xcalloc(size_t n, size_t s) {
    if (n == 0) n = 1;
    if (s == 0) s = 1;
    void *p = calloc(n, s);
    if (!p) oom();
    return p;
}

void *xrealloc(void *p, size_t n) {
    if (n == 0) n = 1;
    void *q = realloc(p, n);
    if (!q) oom();
    return q;
}

char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *r = (char*)xmalloc(n + 1);
    memcpy(r, s, n + 1);
    return r;
}

char *xstrndup(const char *s, size_t n) {
    char *r = (char*)xmalloc(n + 1);
    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

char *read_file_all(const char *path, size_t *len_out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "lslc: cannot open '%s': %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = (char*)xmalloc((size_t)sz + 1);
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';
    if (len_out) *len_out = got;
    return buf;
}

/* Iterative Levenshtein with early cutoff. Case-insensitive. */
int levenshtein(const char *a, const char *b, int cutoff) {
    int la = (int)strlen(a);
    int lb = (int)strlen(b);
    if (la == 0) return lb;
    if (lb == 0) return la;
    if (cutoff > 0 && (la - lb > cutoff || lb - la > cutoff)) return cutoff + 1;

    int *prev = (int*)xmalloc(sizeof(int) * (lb + 1));
    int *curr = (int*)xmalloc(sizeof(int) * (lb + 1));
    for (int j = 0; j <= lb; j++) prev[j] = j;
    for (int i = 1; i <= la; i++) {
        curr[0] = i;
        int row_min = curr[0];
        for (int j = 1; j <= lb; j++) {
            char ca = a[i-1]; if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + 32);
            char cb = b[j-1]; if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + 32);
            int cost = ca == cb ? 0 : 1;
            int x = prev[j] + 1;
            int y = curr[j-1] + 1;
            int z = prev[j-1] + cost;
            int m = x < y ? x : y; if (z < m) m = z;
            curr[j] = m;
            if (m < row_min) row_min = m;
        }
        if (cutoff > 0 && row_min > cutoff) {
            free(prev); free(curr);
            return cutoff + 1;
        }
        int *tmp = prev; prev = curr; curr = tmp;
    }
    int r = prev[lb];
    free(prev); free(curr);
    return r;
}
