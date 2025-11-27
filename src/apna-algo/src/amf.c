#include "amf.h"

#include <netinet/sctp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "utils.h"

// Externalized mutex used by the round-robin function
pthread_mutex_t get_next_amf_mutex = PTHREAD_MUTEX_INITIALIZER;
extern int AMF_CAPACITY;
AMF amfs[MAX_AMFS];

void get_ip(char* ip, int index) {
    snprintf(ip, INET_ADDRSTRLEN, "10.0.0.%d", index + 4);
}

void amf_init_default(void) {
    // Initialize the default AMF entries; caller may modify
    char ip[INET_ADDRSTRLEN];

    for (int i = 0; i < MAX_AMFS; i++) {
        amfs[i].id = i + 1;
        get_ip(ip, i);
        strncpy(amfs[i].ip, ip, sizeof(amfs[i].ip));
        amfs[i].port = PORT;
        amfs[i].active = 1;
        amfs[i].connections = 0;
        pthread_mutex_init(&amfs[i].lock, NULL);
    }
}

int get_active_amf_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_AMFS; i++) {
        if (amfs[i].active) count++;
    }
    log("INFO", "[amf] get_active_amf_count: Active AMF count = %d\n", count);
    return count;
}

AMF* amf_get_by_index(int i) {
    if (i < 0 || i >= MAX_AMFS) {
        log("INFO", "[amf] amf_get_by_index: Invalid index %d\n", i);
        return NULL;
    }
    log("INFO", "[amf] amf_get_by_index: Returning AMF at index %d (id=%d)\n", i, amfs[i].id);
    return &amfs[i];
}

AMF* get_next_amf(void) {
    log("INFO", "[amf] get_next_amf: Choosing next active AMF using least connections algorithm\n");

    AMF* target_amf = NULL;
    pthread_mutex_lock(&get_next_amf_mutex);

    if (get_active_amf_count() == 0) {
        log("INFO", "[amf] get_next_amf: No active AMFs\n");
        pthread_mutex_unlock(&get_next_amf_mutex);
        return NULL;
    }
    // algorithm: argmin_{AMF}{connections} | AMF is active and can take enough load
    int min_conns = INT32_MAX;
    int target_index = -1;
    for (int i = 0; i < MAX_AMFS; i++) {
        if (amfs[i].active && amfs[i].connections < AMF_CAPACITY) {
            if (amfs[i].connections < min_conns) {
                min_conns = amfs[i].connections;
                target_index = i;
            }
        }
    }

    if (target_index != -1) {
        target_amf = &amfs[target_index];
        log("INFO", "[amf] get_next_amf: Selected AMF index %d (id=%d, ip=%s)\n", target_index, amfs[target_index].id, amfs[target_index].ip);
    }
    pthread_mutex_unlock(&get_next_amf_mutex);
    return target_amf;
}

int connect_to_amf(AMF* amf) {
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
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log("INFO", "[amf] connect_to_amf: Failed to connect to %s:%d\n", amf->ip, amf->port);
        close(sock);
        return -1;
    }
    log("INFO", "[amf] connect_to_amf: Successfully connected to %s:%d (sock=%d)\n", amf->ip, amf->port, sock);
    return sock;
}