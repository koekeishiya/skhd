#ifndef SKHD_LOG_H
#define SKHD_LOG_H

#define internal static
#define global   static

global bool verbose;

internal inline void
debug(const char *format, ...)
{
    if (!verbose) return;

    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

internal inline void
warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

internal inline void
error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

#undef internal
#undef global

#endif
