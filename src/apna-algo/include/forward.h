#ifndef FORWARD_H
#define FORWARD_H

#include <pthread.h>
#include <stdbool.h>

#include "amf.h"

#define BUFFER_SIZE 1024
#define MAX_RETRIES 10
#define FORWARD_CONNS_ARRAY_LEN 2 * MAX_CONNECTIONS
#define RECONNECT_BUFFER_TIME 5  // in seconds

typedef struct {
    int source_socket;
    int* destination_socket;  // pointer to fd to allow live replacement
    AMF** current_amf;        // pointer to pointer for live migration
    int live_thread_index;    // index in the live_threads table
    int is_active;
    bool from_gnb;            // true if forwarding to server (AMF), false if to client (gNB)
} forward_info_t;

void forward_init_table(void);
int forward_register_thread(forward_info_t* info);
void forward_unregister_index(int idx);
void* forward_messages(void* arg);
void* handle_gnb_connection(void* arg);

extern forward_info_t* live_threads[FORWARD_CONNS_ARRAY_LEN];
extern pthread_mutex_t live_threads_mutex;

#endif  // FORWARD_H
