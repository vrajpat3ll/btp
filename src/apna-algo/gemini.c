#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_AMFS 3
#define MAX_CONNECTIONS 100  // Increased for a more realistic test
#define DESCALING_INTERVAL_MINUTES 1

// --- Scaling Parameters ---
int AMF_CAPACITY = 10;  // 10
const float HEADROOM_PERCENTAGE = 0.1;
const float THRESHOLD_DOWN = 0.2;
const float THRESHOLD_UP = 0.5;

// --- Global State and Mutexes ---
pthread_mutex_t amf_state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t round_robin_mutex = PTHREAD_MUTEX_INITIALIZER;

int total_conn_count = 0;
int round_robin_index = 0;

typedef struct AMF {
    unsigned int id;
    char ip[16];
    unsigned int port;
    int active;
    int connections;
    pthread_mutex_t lock;
} AMF;

AMF amfs[MAX_AMFS] = {
    {1, "10.0.3.3", 38412, 1, 0, PTHREAD_MUTEX_INITIALIZER},
    {2, "10.0.3.4", 38412, 0, 0, PTHREAD_MUTEX_INITIALIZER},
    {3, "10.0.3.5", 38412, 0, 0, PTHREAD_MUTEX_INITIALIZER}};

// Forwarding thread structure
typedef struct {
    int source_socket;
    int *destination_socket;  // Dynamic socket pointer
    AMF **current_amf;        // Pointer to pointer for live migration
    int live_thread_index;    // FIX: Index in the global array for cleanup
    int is_active;            // FIX: Flag to manage cleanup in the live_threads array
} forward_info_t;

// List of live forwarding threads
forward_info_t *live_threads[MAX_CONNECTIONS];  // Renamed from MAX_LIVE_THREADS
pthread_mutex_t live_threads_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- Helper Functions ---

int get_active_amf_count() {
    int count = 0;
    for (int i = 0; i < MAX_AMFS; i++)
        if (amfs[i].active) count++;
    return count;
}

AMF *get_next_amf_round_robin() {
    AMF *target_amf = NULL;
    pthread_mutex_lock(&round_robin_mutex);
    int active_count = get_active_amf_count();
    if (active_count == 0) {
        pthread_mutex_unlock(&round_robin_mutex);
        return NULL;
    }
    for (int i = 0; i < MAX_AMFS; i++) {
        int idx = (round_robin_index + i) % MAX_AMFS;
        if (amfs[idx].active) {
            target_amf = &amfs[idx];
            round_robin_index = (idx + 1) % MAX_AMFS;
            break;
        }
    }
    pthread_mutex_unlock(&round_robin_mutex);
    return target_amf;
}

void execute_command(char *cmd, char *args[]) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (execvp(cmd, args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        wait(NULL);
    }
}

// --- Forwarding Thread ---
void *forward_messages(void *arg) {
    forward_info_t *info = (forward_info_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;

    while ((nbytes = sctp_recvmsg(info->source_socket, buffer, sizeof(buffer), NULL, 0, NULL, NULL)) > 0) {
        int dest_sock = -1;
        AMF *temp_amf = *(info->current_amf);

        // We lock the AMF just to safely read the socket descriptor
        pthread_mutex_lock(&temp_amf->lock);
        dest_sock = *(info->destination_socket);
        pthread_mutex_unlock(&temp_amf->lock);

        if (dest_sock > 0) {
            sctp_sendmsg(dest_sock, buffer, nbytes, NULL, 0, 0, 0, 0, 0, 0);
        }
    }

    // --- Cleanup ---
    pthread_mutex_lock(&amf_state_mutex);
    if (*info->current_amf) {
        (*info->current_amf)->connections--;
        total_conn_count--;
    }
    pthread_mutex_unlock(&amf_state_mutex);

    // FIX: Mark this thread as inactive in the global list for reuse
    pthread_mutex_lock(&live_threads_mutex);
    info->is_active = 0;
    live_threads[info->live_thread_index] = NULL;  // Clear the slot
    pthread_mutex_unlock(&live_threads_mutex);

    close(info->source_socket);
    close(*(info->destination_socket));  // Close the AMF-side socket

    // FIX: Free all allocated memory
    free(info->destination_socket);
    free(info->current_amf);
    free(info);

    return NULL;
}

int connect_to_amf(AMF *amf) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sock < 0) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(amf->port);
    addr.sin_addr.s_addr = inet_addr(amf->ip);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

void *handle_gnb_connection(void *arg) {
    int gnb_socket = *(int *)arg;
    free(arg);

    AMF *target_amf = get_next_amf_round_robin();
    if (!target_amf) {
        close(gnb_socket);
        return NULL;
    }

    // FIX: Enforce consistent lock order to prevent deadlock
    // Global lock first, then specific lock.
    pthread_mutex_lock(&amf_state_mutex);
    pthread_mutex_lock(&target_amf->lock);

    int amf_sock = connect_to_amf(target_amf);
    if (amf_sock < 0) {
        pthread_mutex_unlock(&target_amf->lock);
        pthread_mutex_unlock(&amf_state_mutex);
        close(gnb_socket);
        return NULL;
    }

    target_amf->connections++;
    total_conn_count++;

    // Unlock in reverse order
    pthread_mutex_unlock(&target_amf->lock);
    pthread_mutex_unlock(&amf_state_mutex);

    // Allocate dynamic structures
    forward_info_t *gnb_to_amf = malloc(sizeof(forward_info_t));
    gnb_to_amf->source_socket = gnb_socket;
    gnb_to_amf->destination_socket = malloc(sizeof(int));
    *(gnb_to_amf->destination_socket) = amf_sock;
    gnb_to_amf->current_amf = malloc(sizeof(AMF *));
    *(gnb_to_amf->current_amf) = target_amf;
    gnb_to_amf->is_active = 1;

    // FIX: Add to live_threads array safely, reusing empty slots
    pthread_mutex_lock(&live_threads_mutex);
    int found_slot = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (live_threads[i] == NULL) {
            live_threads[i] = gnb_to_amf;
            gnb_to_amf->live_thread_index = i;
            found_slot = i;
            break;
        }
    }
    pthread_mutex_unlock(&live_threads_mutex);

    if (found_slot == -1) {
        fprintf(stderr, "Error: Max connections reached. Cannot handle new connection.\n");
        close(gnb_socket);
        close(amf_sock);
        free(gnb_to_amf->destination_socket);
        free(gnb_to_amf->current_amf);
        free(gnb_to_amf);
        return NULL;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, forward_messages, gnb_to_amf);
    pthread_detach(thread);

    return NULL;
}

void scale_up_check() {
    pthread_mutex_lock(&amf_state_mutex);
    int active_count = get_active_amf_count();
    if (active_count == 0) {  // Edge case if all AMFs are somehow inactive
        pthread_mutex_unlock(&amf_state_mutex);
        return;
    }
    int threshold = AMF_CAPACITY * active_count - (int)(AMF_CAPACITY * HEADROOM_PERCENTAGE);

    if (total_conn_count >= threshold) {
        for (int i = 0; i < MAX_AMFS; i++) {
            if (!amfs[i].active) {
                printf("!!! SCALE UP: Load (%d) exceeds threshold (%d). Deploying AMF %d...\n",
                       total_conn_count, threshold, amfs[i].id);
                amfs[i].active = 1;
                char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment",
                               (i == 1) ? "core5g-amf-2-deployment" : "core5g-amf-3-deployment",
                               "--replicas=1", NULL};
                execute_command("kubectl", cmd);
                break;
            }
        }
    }
    // wait();
    pthread_mutex_unlock(&amf_state_mutex);
}

void *descaling_thread(void *arg) {
    while (1) {
        sleep(DESCALING_INTERVAL_MINUTES * 60);

        pthread_mutex_lock(&amf_state_mutex);
        if (get_active_amf_count() <= 1) {
            pthread_mutex_unlock(&amf_state_mutex);
            continue;
        }

        for (int i = 0; i < MAX_AMFS; i++) {
            AMF *old_amf = &amfs[i];
            if (!old_amf->active) continue;

            float util = (old_amf->connections > 0) ? (float)old_amf->connections / AMF_CAPACITY : 0.0;
            if (util < THRESHOLD_DOWN) {
                AMF *new_amf = NULL;
                int min_load = AMF_CAPACITY;
                for (int j = 0; j < MAX_AMFS; j++) {
                    AMF *cand = &amfs[j];
                    if (!cand->active || cand == old_amf) continue;
                    float cand_util = (float)cand->connections / AMF_CAPACITY;
                    if (cand_util >= THRESHOLD_DOWN && cand_util < THRESHOLD_UP && cand->connections < min_load) {
                        min_load = cand->connections;
                        new_amf = cand;
                    }
                }

                if (new_amf) {
                    printf("!!! SCALE DOWN: Migrating from AMF %d to AMF %d\n", old_amf->id, new_amf->id);
                    pthread_mutex_lock(&old_amf->lock);
                    pthread_mutex_lock(&new_amf->lock);

                    pthread_mutex_lock(&live_threads_mutex);
                    for (int t = 0; t < MAX_CONNECTIONS; t++) {
                        if (live_threads[t] == NULL || !live_threads[t]->is_active) continue;

                        forward_info_t *thread_info = live_threads[t];
                        if (*(thread_info->current_amf) == old_amf) {
                            int old_sock = *(thread_info->destination_socket);
                            int new_sock = connect_to_amf(new_amf);

                            if (new_sock > 0) {
                                *(thread_info->current_amf) = new_amf;
                                *(thread_info->destination_socket) = new_sock;
                                // new_amf->connections
                                printf("Migrated connection (gNB sock %d) to new AMF socket %d\n", thread_info->source_socket, new_sock);
                                // FIX: Close the old socket to prevent a file descriptor leak
                                close(old_sock);
                            } else {
                                fprintf(stderr, "Migration failed for gNB sock %d: could not connect to new AMF.\n", thread_info->source_socket);
                                // The connection will likely fail and clean itself up.
                            }
                        }
                    }
                    pthread_mutex_unlock(&live_threads_mutex);

                    new_amf->connections += old_amf->connections;
                    old_amf->connections = 0;
                    old_amf->active = 0;

                    char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment",
                                   (i == 1) ? "core5g-amf-2-deployment" : "core5g-amf-3-deployment",
                                   "--replicas=0", NULL};
                    execute_command("kubectl", cmd);

                    pthread_mutex_unlock(&new_amf->lock);
                    pthread_mutex_unlock(&old_amf->lock);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&amf_state_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // if (argc < 2) {
    //     printf("Usage: %s <amf_capacity>\n", argv[0]);
    //     return 1;
    // }
    // AMF_CAPACITY = atoi(argv[1]);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        live_threads[i] = NULL;
    }

    int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    // ... Add error checking for socket, bind, listen ...
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = inet_addr("10.0.3.1");  // Make this configurable if needed
    listen_addr.sin_port = htons(38412);

    bind(listen_socket, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    listen(listen_socket, MAX_CONNECTIONS);
    printf("Proxy listening on 10.0.3.1:38412 with AMF capacity %d\n", AMF_CAPACITY);

    pthread_t descaling_t;
    pthread_create(&descaling_t, NULL, descaling_thread, NULL);
    pthread_detach(descaling_t);

    while (1) {
        struct sockaddr_in gnb_addr;
        socklen_t addr_len = sizeof(gnb_addr);
        int *gnb_sock = malloc(sizeof(int));
        *gnb_sock = accept(listen_socket, (struct sockaddr *)&gnb_addr, &addr_len);

        if (*gnb_sock < 0) {
            perror("accept failed");
            free(gnb_sock);
            continue;
        }

        scale_up_check();

        pthread_t gnb_thread;
        pthread_create(&gnb_thread, NULL, handle_gnb_connection, gnb_sock);
        pthread_detach(gnb_thread);
    }

    close(listen_socket);
    return 0;
}