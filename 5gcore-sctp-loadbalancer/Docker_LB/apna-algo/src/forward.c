#include "forward.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "amf.h"
#include "scale.h"
#include "utils.h"

pthread_mutex_t live_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
forward_info_t *live_threads[FORWARD_CONNS_ARRAY_LEN];
const RECONNECT_BUFFER_TIME = 30;  // in seconds
struct timeval start_time, end_time;

extern pthread_mutex_t amf_state_mutex;  // declared in scaling.c (shared state)
extern int total_conn_count;             // declared in scaling.c

void forward_init_table(void) {
    log("INFO", "[forward] forward_init_table: Initializing live thread table\n");
    pthread_mutex_lock(&live_threads_mutex);
    for (int i = 0; i < FORWARD_CONNS_ARRAY_LEN; i++) live_threads[i] = NULL;
    pthread_mutex_unlock(&live_threads_mutex);
}

static void cleanup_forward_info(forward_info_t *info) {
    if (!info) return;
    log("INFO", "[forward] cleanup_forward_info: Closing source socket %d\n", info->source_socket);
    close(info->source_socket);
    if (info->destination_socket && *(info->destination_socket) > 0) {
        log("INFO", "[forward] cleanup_forward_info: Closing destination socket %d\n", *(info->destination_socket));
        close(*(info->destination_socket));
    }
    free(info->destination_socket);
    free(info->current_amf);
    free(info);
}

void *forward_messages(void *arg) {
    forward_info_t *info = (forward_info_t *)arg;

    char buffer[BUFFER_SIZE];
    ssize_t nbytes;
    char src_addr[64], dst_addr[64];
    get_ip_port(info->source_socket, src_addr, sizeof(src_addr));

    pthread_mutex_lock(&(*info->current_amf)->lock);
    get_ip_port(*(info->destination_socket), dst_addr, sizeof(dst_addr));
    pthread_mutex_unlock(&(*info->current_amf)->lock);

    log("INFO", "[forward] forward_messages: Started forwarding messages (thread index: %d, %s -> %s)\n",
        info->live_thread_index, src_addr, dst_addr);

    while (1) {
        nbytes = sctp_recvmsg(info->source_socket, buffer, sizeof(buffer), NULL, 0, NULL, NULL);
        int dest_sock = -1;
        AMF *temp_amf = *(info->current_amf);

        // Lock AMF to safely read the socket descriptor if needed

        if (nbytes > 0) {
            pthread_mutex_lock(&temp_amf->lock);
            dest_sock = *(info->destination_socket);
            pthread_mutex_unlock(&temp_amf->lock);
            if (dest_sock > 0) {
                ssize_t sent = sctp_sendmsg(dest_sock, buffer, nbytes, NULL, 0, 0, 0, 0, 0, 0);
                if (sent < 0) {
                    log_perror("[forward] sctp_sendmsg failed");
                    break;
                }
                log("INFO", "[forward] Transferred %zd bytes %s -> %s\n", nbytes, src_addr, dst_addr);
                continue;
            }
        }

        if (nbytes == 0) {
            log("INFO", "[forward] Peer closed connection on socket %d (recv=0)\n", info->source_socket);
            break;
        }

        // nbytes < 0: handle error
        if (errno == ECONNRESET || errno == ENOTCONN || errno == EBADF || errno == EPIPE) {
            log("INFO", "[forward] Detected abrupt disconnect (errno=%d) on socket %d\n", errno, info->source_socket);
            break;
        }

        if (errno == EINTR) continue;  // retry
        log_perror("[forward] sctp_recvmsg failed");
        break;
    }

    log("INFO", "[forward] forward_messages: Connection closed or error on source socket %d\n", info->source_socket);

    // Cleanup connection counts
    pthread_mutex_lock(&amf_state_mutex);
    if (*info->current_amf) {
        (*info->current_amf)->connections--;
        total_conn_count--;
        log("INFO", "[forward] forward_messages: Decremented connection counts for AMF id=%d, total_conn_count=%d\n",
            (*info->current_amf)->id, total_conn_count);
    }
    pthread_mutex_unlock(&amf_state_mutex);

    // Unregister from live table
    pthread_mutex_lock(&live_threads_mutex);
    info->is_active = 0;
    if (info->live_thread_index >= 0 && info->live_thread_index < FORWARD_CONNS_ARRAY_LEN) {
        live_threads[info->live_thread_index] = NULL;
        log("INFO", "[forward] forward_messages: Unregistered thread index %d\n", info->live_thread_index);
    }
    pthread_mutex_unlock(&live_threads_mutex);

    cleanup_forward_info(info);
    return NULL;
}

int forward_register_thread(forward_info_t *info) {
    pthread_mutex_lock(&live_threads_mutex);
    int found_slot = -1;
    for (int i = 0; i < FORWARD_CONNS_ARRAY_LEN; i++) {
        if (live_threads[i] == NULL) {
            live_threads[i] = info;
            info->live_thread_index = i;
            found_slot = i;
            log("INFO", "[forward] forward_register_thread: Registered thread at slot %d\n", i);
            break;
        }
    }
    pthread_mutex_unlock(&live_threads_mutex);
    if (found_slot == -1) {
        log("INFO", "[forward] forward_register_thread: No free slot available\n");
    }
    return found_slot;
}

void forward_unregister_index(int idx) {
    if (idx < 0 || idx >= FORWARD_CONNS_ARRAY_LEN) {
        log("INFO", "[forward] forward_unregister_index: Invalid index %d\n", idx);
        return;
    }
    pthread_mutex_lock(&live_threads_mutex);
    if (live_threads[idx]) {
        live_threads[idx]->is_active = 0;
        live_threads[idx] = NULL;
        log("INFO", "[forward] forward_unregister_index: Unregistered thread at index %d\n", idx);
    }
    pthread_mutex_unlock(&live_threads_mutex);
}

void *handle_gnb_connection(void *arg) {
    int gnb_socket = *(int *)arg;
    free(arg);

    struct sockaddr_in gnb_addr;
    socklen_t addr_len = sizeof(gnb_addr);
    char gnb_ip[INET_ADDRSTRLEN];  // for IPv4, use INET6_ADDRSTRLEN for IPv6
    int gnb_port = -1;

    if (getpeername(gnb_socket, (struct sockaddr *)&gnb_addr, &addr_len) == 0) {
        inet_ntop(AF_INET, &gnb_addr.sin_addr, gnb_ip, sizeof(gnb_ip));
        gnb_port = ntohs(gnb_addr.sin_port);
        log("INFO", "[forward] Handling new gNB connection: socket=%d, IP=%s, port=%d\n", gnb_socket, gnb_ip, gnb_port);
    } else {
        log_perror("[forward] getpeername failed");
        snprintf(gnb_ip, sizeof(gnb_ip), "unknown");
    }
    gettimeofday(&start_time, NULL);

    AMF *target_amf = get_next_amf_round_robin();
    int amf_sock;

    do {
        if (!target_amf) {
            log("INFO", "[forward] handle_gnb_connection: No active AMF available, closing gNB socket %d\n", gnb_socket);
            int i = 0;
            for (; i < MAX_RETRIES && !target_amf; ++i) {
                target_amf = get_next_amf_round_robin();
                if (!target_amf) {
                    log("INFO", "[forward] handle_gnb_connection: Still no active AMF, retrying in 10 seconds...\n");
                    sleep(10);
                }
            }
            if (i >= MAX_RETRIES) {
                close(gnb_socket);
                return NULL;
            }
        }

        extern pthread_mutex_t amf_state_mutex;

        // Consistent locking order: global then specific
        pthread_mutex_lock(&amf_state_mutex);
        pthread_mutex_lock(&target_amf->lock);

        amf_sock = connect_to_amf(target_amf);
        if (amf_sock < 0) {
            log("INFO", "[forward] handle_gnb_connection: Failed to connect to AMF id=%d ip=%s\n",
                target_amf->id, target_amf->ip);
            pthread_mutex_unlock(&target_amf->lock);
            pthread_mutex_unlock(&amf_state_mutex);
            sleep(RECONNECT_BUFFER_TIME);
        }
    } while (amf_sock < 0);

    target_amf->connections++;
    extern int total_conn_count;
    total_conn_count++;
    gettimeofday(&end_time, NULL);
    long latency_us = (end_time.tv_sec - start_time.tv_sec) * 1000000L +
                      (end_time.tv_usec - start_time.tv_usec) / 1000L;
    log("INFO", "[forward] Connection setup latency: gNB socket %d -> AMF id=%d took %ld µs (%.3f ms)\n",
        gnb_socket, target_amf->id, latency_us, latency_us / 1000.0);

    pthread_mutex_unlock(&target_amf->lock);
    pthread_mutex_unlock(&amf_state_mutex);

    // --- GNB -> AMF thread ---
    forward_info_t *gnb_to_amf = malloc(sizeof(forward_info_t));
    if (!gnb_to_amf) {
        log("INFO", "[forward] handle_gnb_connection: Memory allocation failed for gnb_to_amf\n");
        close(gnb_socket);
        close(amf_sock);
        return NULL;
    }

    gnb_to_amf->source_socket = gnb_socket;
    gnb_to_amf->destination_socket = malloc(sizeof(int));
    *(gnb_to_amf->destination_socket) = amf_sock;
    gnb_to_amf->current_amf = (AMF **)malloc(sizeof(AMF *));
    *(gnb_to_amf->current_amf) = target_amf;
    gnb_to_amf->is_active = 1;
    gnb_to_amf->live_thread_index = -1;

    int slot_g2a = forward_register_thread(gnb_to_amf);
    if (slot_g2a == -1) {
        fprintf(stderr, "[forward] handle_gnb_connection: Max connections reached. Cannot handle new connection.\n");
        cleanup_forward_info(gnb_to_amf);
        return NULL;
    }

    pthread_t thread_g2a;
    if (pthread_create(&thread_g2a, NULL, forward_messages, gnb_to_amf) != 0) {
        log_perror("[forward] handle_gnb_connection: pthread_create error (GNB->AMF)");
        forward_unregister_index(slot_g2a);
        cleanup_forward_info(gnb_to_amf);
        return NULL;
    }
    pthread_detach(thread_g2a);
    log("INFO", "[forward] handle_gnb_connection: GNB->AMF forwarding thread started at index %d\n", slot_g2a);

    // --- AMF -> GNB thread ---
    forward_info_t *amf_to_gnb = malloc(sizeof(forward_info_t));
    if (!amf_to_gnb) {
        log("INFO", "[forward] handle_gnb_connection: Memory allocation failed for amf_to_gnb\n");
        return NULL;
    }

    amf_to_gnb->source_socket = amf_sock;
    amf_to_gnb->destination_socket = malloc(sizeof(int));
    *(amf_to_gnb->destination_socket) = gnb_socket;
    amf_to_gnb->current_amf = (AMF **)malloc(sizeof(AMF *));
    *(amf_to_gnb->current_amf) = target_amf;
    amf_to_gnb->is_active = 1;
    amf_to_gnb->live_thread_index = -1;

    int slot_a2g = forward_register_thread(amf_to_gnb);
    if (slot_a2g == -1) {
        fprintf(stderr, "[forward] handle_gnb_connection: Max connections reached. Cannot handle AMF->GNB thread.\n");
        cleanup_forward_info(amf_to_gnb);
        return NULL;
    }

    pthread_t thread_a2g;
    if (pthread_create(&thread_a2g, NULL, forward_messages, amf_to_gnb) != 0) {
        log_perror("[forward] handle_gnb_connection: pthread_create error (AMF->GNB)");
        forward_unregister_index(slot_a2g);
        cleanup_forward_info(amf_to_gnb);
        return NULL;
    }
    pthread_detach(thread_a2g);
    log("INFO", "[forward] handle_gnb_connection: AMF->GNB forwarding thread started at index %d\n", slot_a2g);

    return NULL;
}

void get_ip_port(int sock, char *buf, size_t buflen) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ip_str[INET6_ADDRSTRLEN];
    int port = 0;

    if (getpeername(sock, (struct sockaddr *)&addr, &addr_len) == 0) {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&addr;
            inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
            port = ntohs(s->sin_port);
        } else if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
            inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
            port = ntohs(s->sin6_port);
        } else {
            snprintf(ip_str, sizeof(ip_str), "UnknownAF");
        }
        snprintf(buf, buflen, "%s:%d", ip_str, port);
    } else {
        snprintf(buf, buflen, "unknown");
    }
}