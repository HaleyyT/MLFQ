#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

void die(const char* fmt, ...);
void tracef(bool enabled, int t, const char* fmt, ...);

/* Parse CLI:
   prog <t0> <t1> <t2> <W> <input_file> [--trace]
   returns true on success; sets *trace if present */
bool parse_args(int argc, char** argv, int* t0, int* t1, int* t2, int* W,
                const char** infile, bool* trace);

#endif
