#ifndef DEBUG_H
#define DEBUG_H

/*** Debug define shorthand ***/
#ifdef DEBUG_ALL
#define DEBUG_LOG
#define DEBUG_AUDIO
#endif

/*** Debug log ***/
#ifdef DEBUG_LOG
FILE *debug_log;

inline void init_debug_log(void) {
    debug_log = fopen("romfs:/debug.log", "w");
}

inline void debug_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);

    vprintf(fmt, args);
    vfprintf(debug_log, fmt, args);

    va_end(args);
}
#define printf(fmt, ...) debug_printf(fmt, ##__VA_ARGS__)

#else
#define init_debug_log()
#define debug_printf(fmt, ...)
#endif

#endif