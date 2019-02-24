#ifndef MACOS_TIMING_H
#define MACOS_TIMING_H

#include <stdint.h>
#include <CoreAudio/CoreAudio.h>

#define BEGIN_SCOPED_TIMED_BLOCK(note) \
    do { \
        struct timing_info timing; \
        if (profile) begin_timing(&timing, note)
#define END_SCOPED_TIMED_BLOCK() \
        if (profile) end_timing(&timing); \
    } while (0)

#define BEGIN_TIMED_BLOCK(note) \
    struct timing_info timing; \
    if (profile) begin_timing(&timing, note)
#define END_TIMED_BLOCK() \
    if (profile) end_timing(&timing)

static bool profile;

struct timing_info
{
    char *note;
    uint64_t start;
    uint64_t end;
    float ms;
};

void begin_timing(struct timing_info *timing, char *note);
void end_timing(struct timing_info *timing);

static inline uint64_t
macos_get_wall_clock(void)
{
    uint64_t result = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
    return result;
}

static inline float
macos_get_seconds_elapsed(uint64_t start, uint64_t end)
{
    float result = ((float)(end - start) / 1000.0f) / 1000000.0f;
    return result;
}

static inline float
macos_get_milliseconds_elapsed(uint64_t start, uint64_t end)
{
    float result = 1000.0f * macos_get_seconds_elapsed(start, end);
    return result;
}

void begin_timing(struct timing_info *timing, char *note) {
    timing->note = note;
    timing->start = macos_get_wall_clock();
}

void end_timing(struct timing_info *timing) {
    timing->end = macos_get_wall_clock();
    timing->ms  = macos_get_milliseconds_elapsed(timing->start, timing->end);
    if (timing->note) {
        printf("%6.4fms (%s)\n", timing->ms, timing->note);
    } else {
        printf("%6.4fms\n", timing->ms);
    }
}

#endif
