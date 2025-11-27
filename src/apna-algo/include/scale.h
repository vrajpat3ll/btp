#ifndef SCALE_H
#define SCALE_H

#include "amf.h"

#define DESCALING_INTERVAL_MINUTES 1

extern int AMF_CAPACITY;
extern const float HEADROOM_PERCENTAGE;
extern const float THRESHOLD_DOWN;
extern const float THRESHOLD_UP;

void* descaler(void* arg);
void scale_up(void);

// Shared state visible to forward.c
extern pthread_mutex_t amf_state_mutex;
extern int total_conn_count;

#endif  // SCALING_H