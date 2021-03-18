#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

extern bool audioInit(const char *path);
extern void audioExit(void);

extern bool audioAdvancePlaybackPosition(void);
extern unsigned long audioPlaybackPosition(void);

extern unsigned long audioLength(void);

#endif