#include "amf.h"

#include <netinet/sctp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "utils.h"

// Externalized mutex used by the round-robin function
pthread_mutex_t round_robin_mutex = PTHREAD_MUTEX_INITIALIZER;
static int round_robin_index = 0;
extern int AMF_CAPACITY;
AMF amfs[MAX_AMFS];
int ue2amf_map[] = {
    3, 5, 2, 1, 4, 1, 5, 2, 3, 4, 1, 2, 5, 4, 3, 3, 1, 5, 2, 4, 2, 1, 5, 3, 4,
    2, 2, 4, 5, 1, 3, 3, 5, 4, 2, 1, 1, 5, 3, 4, 2, 2, 5, 1, 3, 4, 4, 2, 1, 5,
    1, 4, 3, 5, 2, 2, 1, 3, 5, 4, 4, 1, 2, 5, 3, 3, 2, 4, 1, 5, 5, 2, 3, 1, 4,
    5, 1, 2, 3, 4, 2, 1, 5, 3, 4, 2, 1, 3, 5, 4, 1, 2, 4, 3, 5, 1, 2, 3, 4, 5,
    4, 3, 1, 2, 5, 5, 3, 2, 4, 1, 1, 5, 4, 3, 2, 2, 1, 4, 5, 3, 3, 2, 1, 5, 4,
    2, 5, 1, 3, 4, 3, 5, 2, 1, 4, 2, 3, 5, 1, 4, 2, 2, 3, 5, 4, 1, 5, 3, 2, 4,
    3, 1, 4, 5, 2, 2, 3, 4, 1, 5, 2, 3, 4, 1, 5, 3, 2, 4, 1, 5, 2, 3, 4, 1, 5,
    1, 2, 5, 3, 4, 5, 2, 1, 3, 4, 2, 5, 1, 3, 4, 2, 5, 1, 3, 4, 2, 1, 5, 3, 4,
    4, 3, 2, 1, 5, 4, 2, 3, 1, 5, 2, 4, 3, 1, 5, 2, 4, 3, 1, 5, 2, 4, 3, 1, 5,
    5, 1, 3, 2, 4, 1, 5, 3, 2, 4, 1, 3, 5, 2, 4, 1, 3, 5, 2, 4, 1, 3, 5, 2, 4,
    2, 4, 1, 5, 3, 2, 4, 1, 3, 5, 2, 1, 4, 3, 5, 2, 1, 4, 3, 5, 2, 1, 4, 3, 5,
    3, 2, 5, 1, 4, 3, 2, 5, 1, 4, 3, 2, 5, 1, 4, 3, 2, 5, 1, 4, 3, 2, 5, 1, 4,
    4, 1, 2, 3, 5, 4, 1, 2, 3, 5, 4, 1, 2, 3, 5, 4, 1, 2, 3, 5, 4, 1, 2, 3, 5,
    5, 3, 4, 2, 1, 5, 3, 4, 2, 1, 5, 3, 4, 2, 1, 5, 3, 4, 2, 1, 5, 3, 4, 2, 1,
    1, 5, 2, 4, 3, 1, 5, 2, 4, 3, 1, 5, 2, 4, 3, 1, 5, 2, 4, 3, 1, 5, 2, 4, 3,
    2, 3, 1, 5, 4, 2, 3, 1, 5, 4, 2, 3, 1, 5, 4, 2, 3, 1, 5, 4, 2, 3, 1, 5, 4,
    3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2,
    4, 5, 1, 3, 2, 4, 5, 1, 3, 2, 4, 5, 1, 3, 2, 4, 5, 1, 3, 2, 4, 5, 1, 3, 2,
    5, 1, 2, 4, 3, 5, 1, 2, 4, 3, 5, 1, 2, 4, 3, 5, 1, 2, 4, 3, 5, 1, 2, 4, 3,
    1, 4, 3, 5, 2, 1, 4, 3, 5, 2, 1, 4, 3, 5, 2, 1, 4, 3, 5, 2, 1, 4, 3, 5, 2};

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

AMF* get_next_amf(const char* ip) {
    log("INFO", "[amf] get_next_amf: Choosing next active AMF using static mapping\n");
    // Try to parse UE id from IP (format expected: a.b.x.y). id = 256*x + y
    int a = 0, b = 0, x = 0, y = 0;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &x, &y) == 4) {
        long ue_id = 256L * (x-1) + y;

        if (ue2amf_map && (size_t)ue_id <= 500) {
            int mapped_amf_id = ue2amf_map[ue_id];
            if (mapped_amf_id >= 1 && mapped_amf_id <= MAX_AMFS) {
                AMF* mapped_amf = amf_get_by_index(mapped_amf_id);
                if (mapped_amf) {
                    if (mapped_amf->active && mapped_amf->connections < AMF_CAPACITY) {
                        log("INFO", "[amf] get_next_amf: UE id=%ld mapped to AMF id=%d (ip=%s)\n", ue_id, mapped_amf->id, mapped_amf->ip);
                        return mapped_amf;
                    } else {
                        log("INFO", "[amf] get_next_amf: Mapped AMF id=%d not available (active=%d connections=%d)\n",
                            mapped_amf->id, mapped_amf->active, mapped_amf->connections);
                        // fallthrough to round-robin fallback
                    }
                }
            } else {
                log("INFO", "[amf] get_next_amf: Mapping for UE id=%ld contains invalid AMF id=%d\n", ue_id, mapped_amf_id);
            }
        } else {
            log("INFO", "[amf] get_next_amf: No mapping entry for UE id=%ld (map size=%zu)\n", ue_id, 500);
        }
    } else {
        log("INFO", "[amf] get_next_amf: Failed to parse IP '%s' for UE id mapping\n", ip);
    }

    AMF* target_amf = NULL;
    {
        pthread_mutex_lock(&round_robin_mutex);
        int active_count = get_active_amf_count();
        if (active_count == 0) {
            log("INFO", "[amf] get_next_amf: No active AMFs\n");
            pthread_mutex_unlock(&round_robin_mutex);
            return NULL;
        }
        for (int i = 0; i < MAX_AMFS; i++) {
            int idx = (round_robin_index + i) % MAX_AMFS;
            if (amfs[idx].active && amfs[idx].connections < AMF_CAPACITY) {
                target_amf = &amfs[idx];
                round_robin_index = (idx + 1) % MAX_AMFS;
                log("INFO", "[amf] get_next_amf: Selected AMF index %d (id=%d, ip=%s)\n", idx, amfs[idx].id, amfs[idx].ip);
                break;
            }
        }

        pthread_mutex_unlock(&round_robin_mutex);
    }
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