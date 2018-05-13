#include "mtouch.h"

struct cached_finger_data *cached_finger_data[256];

void process_cached_finger_data(int identifier)
{
    printf("-- begin processing finger (%d) --\n", identifier);
    for (int j = 0; j < buf_len(cached_finger_data[identifier]); ++j) {
        struct cached_finger_data cached_data = cached_finger_data[identifier][j];
        printf("finger %d, pos %.5f, %.5f\n", cached_data.id, cached_data.pos.x, cached_data.pos.y);
    }
    printf("-- end processing --\n");
}

void multitouch_begin(struct multitouch *multitouch, multitouch_callback *callback)
{
    multitouch->dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(multitouch->dev, callback);
    MTDeviceStart(multitouch->dev, 0);
}
