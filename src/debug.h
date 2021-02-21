#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"

/*** Debug define shorthand ***/
#ifdef DEBUG_ALL
#define DEBUG_LOG
#endif

/*** Debug log ***/
#ifdef DEBUG_LOG
FILE *debug_log;

inline void init_debug_log(void) {
    debug_log = fopen("debug.log", "w");
}

inline void debug_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    fprintf(debug_log, fmt, args);
    va_end(args);
}
#else
inline void init_debug_log(void) {}
inline void debug_printf(const char *fmt, ...) {}
#endif

#endif