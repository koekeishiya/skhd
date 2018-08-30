#include "carbon.h"

#define internal static

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
internal OSStatus
carbon_event_handler(EventHandlerCallRef ref, EventRef event, void *context)
{
    struct carbon_event *carbon = (struct carbon_event *) context;

    ProcessSerialNumber psn;
    if (GetEventParameter(event,
                          kEventParamProcessID,
                          typeProcessSerialNumber,
                          NULL,
                          sizeof(psn),
                          NULL,
                          &psn) != noErr) {
        return -1;
    }

    CFStringRef process_name_ref;
    if (CopyProcessName(&psn, &process_name_ref) == noErr) {
        if (carbon->process_name) {
            free(carbon->process_name);
            carbon->process_name = NULL;
        }

        CFStringLowercase(process_name_ref, CFLocaleGetSystem());
        carbon->process_name = copy_cfstring(process_name_ref);
        CFRelease(process_name_ref);
    }

    return noErr;
}
#pragma clang diagnostic pop

bool carbon_event_init(struct carbon_event *carbon)
{
    carbon->target = GetApplicationEventTarget();
    carbon->handler = NewEventHandlerUPP(carbon_event_handler);
    carbon->type.eventClass = kEventClassApplication;
    carbon->type.eventKind = kEventAppFrontSwitched;
    carbon->process_name = NULL;

    return InstallEventHandler(carbon->target,
                               carbon->handler,
                               1,
                               &carbon->type,
                               carbon,
                               &carbon->handler_ref) == noErr;
}
