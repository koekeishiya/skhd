#ifndef MACOS_TIMING_H
#define MACOS_TIMING_H

#include <stdint.h>
#include <CoreServices/CoreServices.h>
#include <mach/mach_time.h>

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
    double ms;
};

void begin_timing(struct timing_info *timing, char *note);
void end_timing(struct timing_info *timing);

static inline uint64_t
macos_get_wall_clock(void)
{
    return mach_absolute_time();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static inline uint64_t macos_get_nanoseconds_elapsed(uint64_t start, uint64_t end)
{
    uint64_t elapsed = end - start;
    Nanoseconds nano = AbsoluteToNanoseconds(*(AbsoluteTime *) &elapsed);
    return *(uint64_t *) &nano;
}
#pragma clang diagnostic pop

static inline double
macos_get_milliseconds_elapsed(uint64_t start, uint64_t end)
{
    uint64_t ns = macos_get_nanoseconds_elapsed(start, end);
    return (double)(ns / 1000000.0);
}

static inline double
macos_get_seconds_elapsed(uint64_t start, uint64_t end)
{
    uint64_t ns = macos_get_nanoseconds_elapsed(start, end);
    return (double)(ns / 1000000000.0);
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
