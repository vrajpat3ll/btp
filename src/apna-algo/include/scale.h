#ifndef SCALE_H
#define SCALE_H

#include "amf.h"
#include "config.h"

extern int AMF_CAPACITY;

void* descaler(void* arg);
void scale_up(void);

// Shared state visible to forward.c
extern pthread_mutex_t amf_state_mutex;
extern int total_conn_count;

#endif  // SCALING_H