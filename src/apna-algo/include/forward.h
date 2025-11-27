#ifndef FORWARD_H
#define FORWARD_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "amf.h"
#include "config.h"

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

// Helper to get peer IP:port string for a socket (writes into buf)
void get_ip_port(int sock, char* buf, size_t buflen);

extern forward_info_t* live_threads[FORWARD_CONNS_ARRAY_LEN];
extern pthread_mutex_t live_threads_mutex;

#endif  // FORWARD_H
