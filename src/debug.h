#ifndef DEBUG_H
#define DEBUG_H

// Debug flags:
//  Name                    Description
//  ----                    ----
//  DEBUG_BEATMAP           Verbose beatmap loader
//  DEBUG_CONSOLE           Print debug output to a console in the bottom screen
//  DEBUG_OVERLAY           Draw a debug overlay
//  DEBUG_DIRECTOR          Verbose game director
//  DEBUG_PLAYER            Verbose player state machine
//  DEBUG_AUDIO             Verbose audio player
//  DEBUG_LOG               Write output to a log file in the ROMFS
//  DEBUG_NOTE_DRAWING      Verbose note drawing loop
//  DEBUG_AUTO              AI plays the game automatically and perfectly

/*** Debug log ***/
#ifdef DEBUG_LOG
void init_debug_log(void);

int btr_debug_printf(const char *__restrict fmt, ...);
#define printf(fmt, ...) btr_debug_printf(fmt, ##__VA_ARGS__)
#endif

#endif