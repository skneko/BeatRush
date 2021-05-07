#include "debug.h"

#include <stdio.h>
#include <stdarg.h>

static FILE *debug_log;

void init_debug_log(void) {
    debug_log = fopen("sdmc:/debug.log", "w");
}

int btr_debug_printf(const char *__restrict fmt, ...) {
    va_list args;
    int written;

    va_start(args, fmt);

    written = vprintf(fmt, args);
    vfprintf(debug_log, fmt, args);
    fflush(debug_log);

    va_end(args);

    return written;
}