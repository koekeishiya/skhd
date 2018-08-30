#ifndef SKHD_CARBON_H
#define SKHD_CARBON_H

#include <Carbon/Carbon.h>

struct carbon_event
{
    EventTargetRef target;
    EventHandlerUPP handler;
    EventTypeSpec type;
    EventHandlerRef handler_ref;
    char * volatile process_name;
};

bool carbon_event_init(struct carbon_event *carbon);

#endif
