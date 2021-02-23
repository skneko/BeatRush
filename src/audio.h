#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

extern bool audioInit(void);
extern void audioExit(void);

extern int audioPlaybackPosition(void);

#endif