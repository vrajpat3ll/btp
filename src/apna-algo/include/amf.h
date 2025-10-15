#ifndef AMF_H
#define AMF_H

#include <pthread.h>

#define MAX_AMFS 3
#define MAX_CONNECTIONS 100

typedef struct AMF {
    unsigned int id;
    char ip[16];
    unsigned int port;
    int active;
    int connections;
    pthread_mutex_t lock;
} AMF;

void amf_init_default(void);
AMF *get_next_amf_round_robin(void);
int get_active_amf_count(void);
int connect_to_amf(AMF *amf);
AMF *amf_get_by_index(int i);

extern AMF amfs[MAX_AMFS];

#endif  // AMF_H