#ifndef BEATMAP_H
#define BEATMAP_H

#include <stdbool.h>

typedef struct _Note {
    unsigned long position; // ms since song started
    unsigned char type;         // 0 = normal/sustained note
    bool topLane;               // true = top lane, false = bottom lane
    unsigned short duration;    // > 0 means sustained
} Note;

typedef struct _Beatmap {
    int start_offset;               // global compensation
    unsigned short approach_time;   // ms between note appearing and hit
    unsigned int note_count;
    Note *notes;
} Beatmap;

extern Beatmap *beatmap_load_from_file(const char *path);

#ifdef DEBUG_BEATMAP
extern void beatmap_print(const Beatmap *const beatmap);
#endif

#endif