#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"

void die(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

void tracef(bool enabled, int t, const char* fmt, ...) {
    if (!enabled) return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "t=%d ", t);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

bool parse_args(int argc, char** argv, int* t0, int* t1, int* t2, int* W,
                const char** infile, bool* trace) {
    if (argc < 6) return false;
    *t0 = atoi(argv[1]);
    *t1 = atoi(argv[2]);
    *t2 = atoi(argv[3]);
    *W  = atoi(argv[4]);
    *infile = argv[5];
    *trace = false;
    for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0) *trace = true;
    }
    return true;
}
