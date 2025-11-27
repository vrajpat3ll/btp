#ifndef SCALE_H
#define SCALE_H

#include "amf.h"
#include "config.h"

/* The implementation file (`src/scale.c`) defines the following variables:
 *   int AMF_CAPACITY; // initialized from DEFAULT_AMF_CAPACITY
 * and uses the macros HEADROOM_PERCENTAGE, THRESHOLD_DOWN, THRESHOLD_UP
 * defined in `config.h`.
 */
extern int AMF_CAPACITY;

void* descaler(void* arg);
void scale_up(void);

// Shared state visible to forward.c
extern pthread_mutex_t amf_state_mutex;
extern int total_conn_count;

#endif  // SCALING_H