#ifndef AMF_H
#define AMF_H

#include <arpa/inet.h>
#include <pthread.h>

#include "config.h"

typedef struct AMF {
    unsigned int id;
    char ip[INET_ADDRSTRLEN];
    unsigned int port;
    int active;
    int connections;
    pthread_mutex_t lock;
} AMF;

extern AMF amfs[MAX_AMFS];

void amf_init_default(void);
AMF* get_next_amf(const char* ip);
int get_active_amf_count(void);
int connect_to_amf(AMF* amf);
AMF* amf_get_by_index(int i);

#endif  // AMF_H