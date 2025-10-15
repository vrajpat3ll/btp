#include "amf.h"
#include "utils.h"

#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Externalized mutex used by the round-robin function
pthread_mutex_t round_robin_mutex = PTHREAD_MUTEX_INITIALIZER;
static int round_robin_index = 0;
extern int AMF_CAPACITY;
AMF amfs[MAX_AMFS];

void amf_init_default(void) {
    // Initialize the default AMF entries; caller may modify
    amfs[0].id = 1;
    strncpy(amfs[0].ip, "10.0.3.3", sizeof(amfs[0].ip));
    amfs[0].port = 38412;
    amfs[0].active = 1;
    amfs[0].connections = 0;
    pthread_mutex_init(&amfs[0].lock, NULL);

    amfs[1].id = 2;
    strncpy(amfs[1].ip, "10.0.3.4", sizeof(amfs[1].ip));
    amfs[1].port = 38412;
    amfs[1].active = 0;
    amfs[1].connections = 0;
    pthread_mutex_init(&amfs[1].lock, NULL);
    
    amfs[2].id = 3;
    strncpy(amfs[2].ip, "10.0.3.5", sizeof(amfs[2].ip));
    amfs[2].port = 38412;
    amfs[2].active = 0;
    amfs[2].connections = 0;
    pthread_mutex_init(&amfs[2].lock, NULL);
}

int get_active_amf_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_AMFS; i++) {
        if (amfs[i].active) count++;
    }
    log("INFO", "[amf] get_active_amf_count: Active AMF count = %d\n", count);
    return count;
}

AMF *amf_get_by_index(int i) {
    if (i < 0 || i >= MAX_AMFS) {
        log("INFO", "[amf] amf_get_by_index: Invalid index %d\n", i);
        return NULL;
    }
    log("INFO", "[amf] amf_get_by_index: Returning AMF at index %d (id=%d)\n", i, amfs[i].id);
    return &amfs[i];
}

AMF *get_next_amf_round_robin(void) {
    log("INFO", "[amf] get_next_amf_round_robin: Choosing next active AMF using round robin\n");

    AMF *target_amf = NULL;
    pthread_mutex_lock(&round_robin_mutex);
    int active_count = get_active_amf_count();
    if (active_count == 0) {
        log("INFO", "[amf] get_next_amf_round_robin: No active AMFs\n");
        pthread_mutex_unlock(&round_robin_mutex);
        return NULL;
    }
    for (int i = 0; i < MAX_AMFS; i++) {
        int idx = (round_robin_index + i) % MAX_AMFS;
        if (amfs[idx].active && amfs[idx].connections < AMF_CAPACITY) {
            target_amf = &amfs[idx];
            round_robin_index = (idx + 1) % MAX_AMFS;
            log("INFO", "[amf] get_next_amf_round_robin: Selected AMF index %d (id=%d, ip=%s)\n", idx, amfs[idx].id, amfs[idx].ip);
            break;
        }
    }

    pthread_mutex_unlock(&round_robin_mutex);
    return target_amf;
}

int connect_to_amf(AMF *amf) {
    log("INFO", "[amf] connect_to_amf: Connecting to AMF id=%d ip=%s port=%d\n", amf->id, amf->ip, amf->port);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sock < 0) {
        log("INFO", "[amf] connect_to_amf: Failed to create socket\n");
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(amf->port);
    addr.sin_addr.s_addr = inet_addr(amf->ip);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log("INFO", "[amf] connect_to_amf: Failed to connect to %s:%d\n", amf->ip, amf->port);
        close(sock);
        return -1;
    }
    log("INFO", "[amf] connect_to_amf: Successfully connected to %s:%d (sock=%d)\n", amf->ip, amf->port, sock);
    return sock;
}