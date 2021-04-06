#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

extern bool audioInit(void);
extern void audioExit(void);

extern bool audioAdvancePlaybackPosition(void);
extern unsigned long audioPlaybackPosition(void);

extern unsigned long audioLength(void);

extern bool audioSetSong(const char *path);
extern void audioPause(void);
extern void audioPlay(void);
extern bool audioIsPaused(void);

#endif