#include "event_tap.h"

bool event_tap_enabled(struct event_tap *event_tap)
{
    bool result = (event_tap->handle && CGEventTapIsEnabled(event_tap->handle));
    return result;
}

bool event_tap_begin(struct event_tap *event_tap, event_tap_callback *callback)
{
    event_tap->handle = CGEventTapCreate(kCGSessionEventTap,
                                         kCGHeadInsertEventTap,
                                         kCGEventTapOptionDefault,
                                         event_tap->mask,
                                         callback,
                                         event_tap);

    bool result = event_tap_enabled(event_tap);
    if(result) {
        event_tap->runloop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                                                  event_tap->handle,
                                                                  0);
        CFRunLoopAddSource(CFRunLoopGetMain(), event_tap->runloop_source, kCFRunLoopCommonModes);
    }

    return result;
}

void event_tap_end(struct event_tap *event_tap)
{
    if(event_tap_enabled(event_tap)) {
        CGEventTapEnable(event_tap->handle, false);
        CFMachPortInvalidate(event_tap->handle);
        CFRunLoopRemoveSource(CFRunLoopGetMain(), event_tap->runloop_source, kCFRunLoopCommonModes);
        CFRelease(event_tap->runloop_source);
        CFRelease(event_tap->handle);
        event_tap->handle = NULL;
    }
}
