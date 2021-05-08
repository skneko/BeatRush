#ifndef BEATMAP_H
#define BEATMAP_H

#include <stdbool.h>

typedef struct _Note {
    unsigned long position;     // ms since song started
    unsigned char type;         // 0 = normal/sustained note
    bool topLane;               // true = top lane, false = bottom lane
    unsigned short duration;    // > 0 means sustained
    bool hidden;
} Note;

typedef struct _BeatmapMetaInfo {
    char *song_name;
    char *artist;
    char *difficulty_name;
} BeatmapMetaInfo;

typedef struct _Beatmap {
    int start_offset;               // global compensation
    unsigned short approach_time;   // ms between note appearing and hit
    unsigned int note_count;
    BeatmapMetaInfo *meta_info;
    Note *notes;
} Beatmap;

extern Beatmap *beatmap_load_from_file(const char *path);

#endif