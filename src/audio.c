/*
 * Fast, threaded Opus audio streaming example using libopusfile
 * for libctru on Nintendo 3DS
 * 
 *     Originally written by Lauren Kelly (thejsa) with lots of help
 * from mtheall, who re-architected the decoding and buffer logic to be
 * much more efficient as well as overall making the code half decent :)
 * 
 *     Thanks also to David Gow for his example code, which is in the
 * public domain & explains in excellent detail how to use libopusfile:
 * https://davidgow.net/hacks/opusal.html
 * 
 * Last update: 2020-05-16
 */

/*
 * Modified by skneko for Beat Rush
 * 2021-02-21
 * 
 * Kindly taken from <https://github.com/devkitPro/3ds-examples/tree/master/audio/opus-decoding>
 * original code belongs to its authors
 */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include "audio.h"
#include "debug.h"

#include <3ds.h>
#include <opusfile.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ---- DEFINITIONS ----

static const int SAMPLE_RATE = 48000;            // Opus is fixed at 48kHz
static const int SAMPLES_PER_BUF = SAMPLE_RATE * 30 / 1000;  // 30ms buffer
static const int CHANNELS_PER_SAMPLE = 2;        // We ask libopusfile for
                                                 // stereo output; it will down
                                                 // -mix for us as necessary.

static const int THREAD_AFFINITY = -1;           // Execute thread on any core
static const int THREAD_STACK_SZ = 32 * 1024;    // 32kB stack for audio thread

static const size_t WAVEBUF_SIZE = SAMPLES_PER_BUF * CHANNELS_PER_SAMPLE
    * sizeof(int16_t);                           // Size of NDSP wavebufs

// ---- END DEFINITIONS ----

OggOpusFile *audioFile = NULL;

volatile int playheadPosition = 0;  // milliseconds since playback started
int songTime = 0;
int lastReportedPlayheadPosition = 0;
u64 previousFrameTime;
#ifdef DEBUG_AUDIO
TickCounter playbackTimer;
#endif

ndspWaveBuf s_waveBufs[3];
int16_t *s_audioBuffer = NULL;

Thread audioThread;
LightEvent s_event;
volatile bool s_song_ongoing = false;   // Song has started but not finished
volatile bool s_paused = false;
volatile bool s_quit = false;  // Quit flag

// ---- HELPER FUNCTIONS ----

// Retrieve strings for libopusfile errors
// Sourced from David Gow's example code: https://davidgow.net/files/opusal.cpp
const char *opusStrError(int error)
{
    switch(error) {
        case OP_FALSE:
            return "OP_FALSE: A request did not succeed.";
        case OP_HOLE:
            return "OP_HOLE: There was a hole in the page sequence numbers.";
        case OP_EREAD:
            return "OP_EREAD: An underlying read, seek or tell operation "
                   "failed.";
        case OP_EFAULT:
            return "OP_EFAULT: A NULL pointer was passed where none was "
                   "expected, or an internal library error was encountered.";
        case OP_EIMPL:
            return "OP_EIMPL: The stream used a feature which is not "
                   "implemented.";
        case OP_EINVAL:
            return "OP_EINVAL: One or more parameters to a function were "
                   "invalid.";
        case OP_ENOTFORMAT:
            return "OP_ENOTFORMAT: This is not a valid Ogg Opus stream.";
        case OP_EBADHEADER:
            return "OP_EBADHEADER: A required header packet was not properly "
                   "formatted.";
        case OP_EVERSION:
            return "OP_EVERSION: The ID header contained an unrecognised "
                   "version number.";
        case OP_EBADPACKET:
            return "OP_EBADPACKET: An audio packet failed to decode properly.";
        case OP_EBADLINK:
            return "OP_EBADLINK: We failed to find data we had seen before or "
                   "the stream was sufficiently corrupt that seeking is "
                   "impossible.";
        case OP_ENOSEEK:
            return "OP_ENOSEEK: An operation that requires seeking was "
                   "requested on an unseekable stream.";
        case OP_EBADTIMESTAMP:
            return "OP_EBADTIMESTAMP: The first or last granule position of a "
                   "link failed basic validity checks.";
        default:
            return "Unknown error.";
    }
}

// ---- END HELPER FUNCTIONS ----

// Audio initialisation code
// This sets up NDSP and our primary audio buffer
bool audioInitHardware(void) {
    // Setup NDSP
    ndspChnReset(0);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(0, SAMPLE_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

    // Allocate audio buffer
    const size_t bufferSize = WAVEBUF_SIZE * ARRAY_SIZE(s_waveBufs);
    s_audioBuffer = (int16_t *)linearAlloc(bufferSize);
    if(!s_audioBuffer) {
        printf("Failed to allocate audio buffer.\n");
        return false;
    }

    // Setup waveBufs for NDSP
    memset(&s_waveBufs, 0, sizeof(s_waveBufs));
    int16_t *buffer = s_audioBuffer;

    for(size_t i = 0; i < ARRAY_SIZE(s_waveBufs); ++i) {
        s_waveBufs[i].data_vaddr = buffer;
        s_waveBufs[i].status     = NDSP_WBUF_DONE;

        buffer += WAVEBUF_SIZE / sizeof(buffer[0]);
    }

    return true;
}

// Audio de-initialisation code
// Stops playback and frees the primary audio buffer
void audioExitHardware(void) {
    ndspChnReset(0);
    linearFree(s_audioBuffer);
}

// Main audio decoding logic
// This function pulls and decodes audio samples from opusFile_ to fill waveBuf_
bool fillBuffer(OggOpusFile *opusFile_, ndspWaveBuf *waveBuf_) {
    #ifdef DEBUG_AUDIO
    // Setup timer for performance stats
    TickCounter timer;
    osTickCounterStart(&timer);
    #endif  // DEBUG

    // Decode samples until our waveBuf is full
    int totalSamples = 0;
    while(totalSamples < SAMPLES_PER_BUF) {
        int16_t *buffer = waveBuf_->data_pcm16 + (totalSamples *
            CHANNELS_PER_SAMPLE);
        const size_t bufferSize = (SAMPLES_PER_BUF - totalSamples) *
            CHANNELS_PER_SAMPLE;

        // Decode bufferSize samples from opusFile_ into buffer,
        // storing the number of samples that were decoded (or error)
        const int samples = op_read_stereo(opusFile_, buffer, bufferSize);
        if(samples <= 0) {
            if(samples == 0) break;  // No error here

            printf("op_read_stereo: error %d (%s)", samples,
                   opusStrError(samples));
            break;
        }
        
        totalSamples += samples;
    }

    // If no samples were read in the last decode cycle, we're done
    if(totalSamples == 0) {
        #ifdef DEBUG_AUDIO
        osTickCounterUpdate(&playbackTimer);
        printf("Audio playback complete (%.3lf ms).\n", osTickCounterRead(&playbackTimer));
        #else
        printf("Audio playback complete.\n");
        #endif

        s_song_ongoing = false;
        return false;
    }

    // Pass samples to NDSP
    waveBuf_->nsamples = totalSamples;
    ndspChnWaveBufAdd(0, waveBuf_);
    DSP_FlushDataCache(waveBuf_->data_pcm16,
        totalSamples * CHANNELS_PER_SAMPLE * sizeof(int16_t));

    playheadPosition += totalSamples * 1000.0 / SAMPLE_RATE;

    #ifdef DEBUG_AUDIO
    // Print timing info
    osTickCounterUpdate(&timer);
    printf("fillBuffer %lfms in %lfms - %lf\n", totalSamples * 1000.0 / SAMPLE_RATE,
           osTickCounterRead(&timer), osTickCounterRead(&playbackTimer));
    #endif  // DEBUG

    return true;
}

// NDSP audio frame callback
// This signals the audioThread to decode more things
// once NDSP has played a sound frame, meaning that there should be
// one or more available waveBufs to fill with more data.
void audioCallback(void *const nul_) {
    (void)nul_;  // Unused

    if(s_quit) { // Quit flag
        return;
    }
    
    LightEvent_Signal(&s_event);
}

// Audio thread
// This handles calling the decoder function to fill NDSP buffers as necessary
void audioThreadRoutine(void *const opusFile_) {
    OggOpusFile *const opusFile = (OggOpusFile *)opusFile_;

    while(!s_quit) {  // Whilst the quit flag is unset,
                      // search our waveBufs and fill any that aren't currently
                      // queued for playback (i.e, those that are 'done')
        if (!s_paused) {
            for(size_t i = 0; i < ARRAY_SIZE(s_waveBufs); ++i) {
                if(s_waveBufs[i].status != NDSP_WBUF_DONE) {
                    continue;
                }
                
                if(!fillBuffer(opusFile, &s_waveBufs[i])) {   // Playback complete
                    s_song_ongoing = false;
                    return;
                }
            }
        }

        // Wait for a signal that we're needed again before continuing,
        // so that we can yield to other things that want to run
        // (Note that the 3DS uses cooperative threading)
        LightEvent_Wait(&s_event);
    }
    
    s_song_ongoing = false;
}

bool audioInit(void) {
    ndspInit();
    
    // Setup LightEvent for synchronisation of audioThread
    LightEvent_Init(&s_event, RESET_ONESHOT);

    printf("Initalizing audio subsystem\n"
        "\n"
        "Using %d waveBufs, each of length %d bytes\n"
        "    (%d samples; %.2lf ms @ %d Hz)\n",
        ARRAY_SIZE(s_waveBufs), WAVEBUF_SIZE, SAMPLES_PER_BUF,
        SAMPLES_PER_BUF * 1000.0 / SAMPLE_RATE, SAMPLE_RATE);

    // Attempt audioInitHardware
    if(!audioInitHardware()) {
        printf("Failed to initialize audio.\n");
        return false;
    }

    // Set the ndsp sound frame callback which signals our audioThread
    ndspSetCallback(audioCallback, NULL);

    s_song_ongoing = false;

    return true;
}

bool audioSetSong(const char *path) {
    printf("Loading audio data from path: %s\n", path);

    // Open the Opus audio file
    int error = 0;
    audioFile = op_open_file(path, &error);
    if (error) {
        printf("Failed to open file: error %d (%s).\n", error,
               opusStrError(error));
        return false;
    }

    ogg_int64_t totalSamples = op_pcm_total(audioFile, -1);
    printf("Total samples: %lld (%.2lf ms)\n", totalSamples, totalSamples * 1000.0 / SAMPLE_RATE);

    /* Spawn audio thread */
    // Set the thread priority to the main thread's priority ...
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    // ... then subtract 1, as lower number => higher actual priority ...
    priority -= 1;
    // ... finally, clamp it between 0x18 and 0x3F to guarantee that it's valid.
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    // Start the thread, passing our audioFile as an argument.
    audioThread = threadCreate(audioThreadRoutine, audioFile,
                                         THREAD_STACK_SZ, priority,
                                         THREAD_AFFINITY, false);
    printf("Created audio thread: %p.\n", audioThread);

    previousFrameTime = osGetTime();
    lastReportedPlayheadPosition = 0;
    s_song_ongoing = true;
    s_paused = true;

    #ifdef DEBUG_AUDIO
    osTickCounterStart(&playbackTimer);
    #endif

    return true;
}

void audioExit(void) {
    // Signal audio thread to quit
    s_quit = true;
    LightEvent_Signal(&s_event);

    // Free the audio thread
    threadJoin(audioThread, UINT64_MAX);
    threadFree(audioThread);

    // Cleanup audio things and de-init platform features
    audioExitHardware();
    ndspExit();
    op_free(audioFile);
}

bool audioAdvancePlaybackPosition(void) {
    int currentTime = osGetTime();
    int dt = currentTime - previousFrameTime;
    previousFrameTime = currentTime;

    if (s_song_ongoing && !s_paused) {
        songTime += dt;
        
        if (playheadPosition != lastReportedPlayheadPosition) {
            songTime = (songTime + playheadPosition) / 2;
            lastReportedPlayheadPosition = playheadPosition;
        }
        
        return true;
    } else {
        return false;
    }
}

unsigned long audioPlaybackPosition(void) {
    return songTime;
}

unsigned long audioLength(void) {
    return op_pcm_total(audioFile, -1) * 1000 / SAMPLE_RATE;
}

void audioPause(void) {
    s_paused = true;
}

void audioPlay(void) {
    s_paused = false;
}

bool audioIsPaused(void) {
    return s_paused;
}