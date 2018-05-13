#include "mtouch.h"

struct cached_finger_data *cached_finger_data[256];

void process_cached_finger_data(int identifier)
{
    printf("-- begin processing finger (%d) --\n", identifier);
    int frame_count = buf_len(cached_finger_data[identifier]);
    for (int i = 0; i < frame_count; ++i) {
        struct cached_finger_data cached_data = cached_finger_data[identifier][i];
        printf("finger: %d, pos: (%.5f, %.5f), pressure: %.5f\n",
                cached_data.id,
                cached_data.pos.x, cached_data.pos.y,
                cached_data.pressure);
    }
    printf("-- end processing (%d frames) --\n", frame_count);
}

void multitouch_begin(struct multitouch *multitouch, multitouch_callback *callback)
{
    multitouch->dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(multitouch->dev, callback);
    MTDeviceStart(multitouch->dev, 0);
}
