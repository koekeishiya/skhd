#ifndef SKHD_MTHOUCH_H
#define SKHD_MTHOUCH_H

struct mt_point;
struct mt_readout;
struct finger;

#define MULTITOUCH_CALLBACK(name) int name(int device, struct finger *data, int num_fingers, double timestamp, int frame)
typedef MULTITOUCH_CALLBACK(multitouch_callback);

typedef void *MTDeviceRef;
typedef int (*MTContactCallbackFunction)(int, struct finger *, int, double, int);

extern MTDeviceRef MTDeviceCreateDefault();
extern void MTRegisterContactFrameCallback(MTDeviceRef, MTContactCallbackFunction);
extern void MTDeviceStart(MTDeviceRef, int);

struct mt_point
{
    float x;
    float y;
};

struct mt_readout
{
    struct mt_point pos;
    struct mt_point vel;
};

struct finger {
    int frame;
    double timestamp;
    int identifier, state, foo3, foo4;
    struct mt_readout normalized;
    float size;
    int zero1;
    float angle, major_axis, minor_axis;
    struct mt_readout mm;
    int zero2[2];
    float unk2;
};

struct cached_finger_data
{
    int id;
    struct mt_point pos;
    float pressure;
};

struct multitouch
{
    MTDeviceRef dev;
};

void process_cached_finger_data(int identifier);
void multitouch_begin(struct multitouch *multitouch, multitouch_callback *callback);

#endif
