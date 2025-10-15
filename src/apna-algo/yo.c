#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_AMFS 3
#define MAX_CONNECTIONS 10
#define DESCALING_INTERVAL_MINUTES 1  // X minutes for descaling check

// --- Scaling Parameters ---
int AMF_CAPACITY;                       // C
const float HEADROOM_PERCENTAGE = 0.1;  // H is derived from this
const float THRESHOLD_DOWN = 0.15;      // T_D (15% utilization)
const float THRESHOLD_UP = 0.25;        // T_U (25% utilization)

// --- Global State and Mutexes ---
pthread_mutex_t amf_state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t round_robin_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *amf_locks[MAX_AMFS];  // For migration locking

int total_conn_count = 0;
int round_robin_index = 0;  // RR_Index

FILE *fp, *fp_amf;

// --- Data Structure Updates ---
typedef struct {
    unsigned int id;
    char ip[16];
    unsigned int port;
    int active;             // 1 for active, 0 for inactive
    int connections;        // AMF_Load[i]
    pthread_mutex_t *lock;  // Lock for migration
} AMF;

// TODO: Dynamically manage AMF array if needed
AMF amfs[MAX_AMFS] = {
    {1, "10.0.3.3", 38412, 1, 0, NULL},  // AMF1 starts active
    {2, "10.0.3.4", 38412, 0, 0, NULL},
    {3, "10.0.3.5", 38412, 0, 0, NULL}};

void handle_sigint(int sig) {
    // Cleanup code...
    if (fp != NULL)
        fclose(fp);

    if (fp_amf != NULL) 
        fclose(fp_amf);

    for (int i = 0; i < MAX_AMFS; i++) {
        if (amf_locks[i]) pthread_mutex_destroy(amf_locks[i]);
    }
    exit(0);
}

typedef struct {
    int source_socket;
    int destination_socket;
    AMF *source_amf;       // Used for load decrement on connection close
    AMF *destination_amf;  // New AMF for migration
} forward_info_t;

// --- Helper Functions ---

int get_active_amf_count() {
    int count = 0;
    for (int i = 0; i < MAX_AMFS; i++) {
        if (amfs[i].active) {
            count++;
        }
    }
    return count;
}

// Round Robin function for new connections
AMF *get_next_amf_round_robin() {
    AMF *target_amf = NULL;
    pthread_mutex_lock(&round_robin_mutex);

    int active_count = get_active_amf_count();
    if (active_count == 0) {
        pthread_mutex_unlock(&round_robin_mutex);
        return NULL;
    }

    // Iterate up to MAX_AMFS times to ensure we find an active AMF
    for (int i = 0; i < MAX_AMFS; i++) {
        int index = (round_robin_index + i) % MAX_AMFS;
        if (amfs[index].active) {
            target_amf = &amfs[index];
            round_robin_index = (index + 1) % MAX_AMFS;
            break;
        }
    }

    pthread_mutex_unlock(&round_robin_mutex);
    return target_amf;
}

void execute_command(char *cmd, char *args[]) {
    // Simplified, non-blocking execution for demonstration
    printf("Executing command: %s (args[3]=%s)\n", cmd, args[3]);
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(cmd, args) == -1) {
            // Error handling, not shown for brevity
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process: non-blocking, we don't wait for simplicity
    }
}

// Function to forward messages from source to destination, and decrement load on close
void *forward_messages(void *arg) {
    forward_info_t *info = (forward_info_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;

    while ((nbytes = sctp_recvmsg(info->source_socket, buffer, sizeof(buffer), NULL, 0, NULL, NULL)) > 0) {
        sctp_sendmsg(info->destination_socket, buffer, nbytes, NULL, 0, 0, 0, 0, 0, 0);
    }

    // --- Critical Section: Load Decrement ---
    pthread_mutex_lock(&amf_state_mutex);
    if (info->source_amf) {
        info->source_amf->connections--;
        total_conn_count--;
        printf("Connection closed. Load on AMF %d: %d. Total load: %d\n",
               info->source_amf->id, info->source_amf->connections, total_conn_count);
    }
    pthread_mutex_unlock(&amf_state_mutex);
    // --- End Critical Section ---

    close(info->source_socket);
    close(info->destination_socket);
    free(info);
    return NULL;
}

// Function to handle each gNB connection
void *handle_gnb_connection(void *arg) {
    int gnb_socket = *(int *)arg;
    free(arg);

    // Round-Robin Selection
    AMF *target_amf = get_next_amf_round_robin();
    if (target_amf == NULL) {
        printf("No active AMFs available for connection!\n");
        close(gnb_socket);
        return NULL;
    }

    // --- Critical Section: Lock for Target AMF ---
    // Acquire lock to prevent migration during connection setup
    pthread_mutex_lock(target_amf->lock);

    int amf_socket;
    struct sockaddr_in amf_addr;
    pthread_t gnb_to_amf_thread, amf_to_gnb_thread;
    forward_info_t *gnb_to_amf_info, *amf_to_gnb_info;

    // Connect to the selected AMF
    amf_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    // ... (socket setup and connection logic omitted for brevity, similar to original) ...
    // Assume connection is successful for a simple demonstration

    struct sockaddr_in check_addr;
    memset(&check_addr, 0, sizeof(check_addr));
    check_addr.sin_family = AF_INET;
    check_addr.sin_addr.s_addr = inet_addr(target_amf->ip);
    check_addr.sin_port = htons(target_amf->port);

    int connect_status = connect(amf_socket, (struct sockaddr *)&check_addr, sizeof(check_addr));

    if (connect_status < 0) {
        printf("AMF %d is down. Connection failed.\n", target_amf->id);
        // Do not increment load if connection fails
        pthread_mutex_unlock(target_amf->lock);
        close(gnb_socket);
        return NULL;
    }

    // --- Critical Section: Load Increment ---
    pthread_mutex_lock(&amf_state_mutex);
    target_amf->connections++;  // AMF_Load[target]++
    total_conn_count++;         // Conn_Count++
    printf("Connection successful to AMF %d. Current load: %d. Total load: %d\n",
           target_amf->id, target_amf->connections, total_conn_count);
    pthread_mutex_unlock(&amf_state_mutex);
    // --- End Critical Section ---

    // Create threads to handle message forwarding
    gnb_to_amf_info = malloc(sizeof(forward_info_t));
    gnb_to_amf_info->source_socket = gnb_socket;
    gnb_to_amf_info->destination_socket = amf_socket;
    gnb_to_amf_info->source_amf = target_amf;  // Pass AMF info for load decrement
    pthread_create(&gnb_to_amf_thread, NULL, forward_messages, gnb_to_amf_info);

    amf_to_gnb_info = malloc(sizeof(forward_info_t));
    amf_to_gnb_info->source_socket = amf_socket;
    amf_to_gnb_info->destination_socket = gnb_socket;
    amf_to_gnb_info->source_amf = target_amf;  // Pass AMF info for load decrement
    pthread_create(&amf_to_gnb_thread, NULL, forward_messages, amf_to_gnb_info);

    // Release lock *after* creating forwarding threads
    pthread_mutex_unlock(target_amf->lock);

    pthread_join(gnb_to_amf_thread, NULL);
    pthread_join(amf_to_gnb_thread, NULL);

    close(gnb_socket);
    close(amf_socket);
    return NULL;
}

// --- Scaling Up Thread (Executed on new connection) ---
void scale_up_check() {
    pthread_mutex_lock(&amf_state_mutex);
    int active_count = get_active_amf_count();
    int scale_up_threshold = AMF_CAPACITY * active_count - (int)(AMF_CAPACITY * HEADROOM_PERCENTAGE);

    if (total_conn_count >= scale_up_threshold) {
        for (int i = 0; i < MAX_AMFS; i++) {
            if (!amfs[i].active) {
                // Step 2 & 3: Deploy New AMF and Update Active Set
                printf("!!! SCALE UP: Load (%d) exceeds threshold (%d). Deploying AMF %d...\n",
                       total_conn_count, scale_up_threshold, amfs[i].id);
                amfs[i].active = 1;
                char replica_str[5];
                sprintf(replica_str, "%d", 1);
                char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment",
                               (i == 1) ? "core5g-amf-2-deployment" : "core5g-amf-3-deployment",
                               "--replicas=1", NULL};
                execute_command("kubectl", cmd);

                // Reset round robin index to distribute new load
                pthread_mutex_lock(&round_robin_mutex);
                round_robin_index = 0;
                pthread_mutex_unlock(&round_robin_mutex);

                break;  // Scale up only one AMF at a time
            }
        }
    }
    pthread_mutex_unlock(&amf_state_mutex);
}

// --- Scaling Down Thread ---
void *descaling_thread(void *arg) {
    while (1) {
        sleep(DESCALING_INTERVAL_MINUTES * 60);

        pthread_mutex_lock(&amf_state_mutex);
        int active_count = get_active_amf_count();

        if (active_count > 1) {
            for (int i = 0; i < MAX_AMFS; i++) {
                AMF *amf_old = &amfs[i];
                if (!amf_old->active) continue;

                float utilization = (float)amf_old->connections / AMF_CAPACITY;

                // Step 2: Check De-scale Threshold
                if (utilization < THRESHOLD_DOWN) {
                    AMF *amf_new = NULL;
                    int min_load = AMF_CAPACITY;  // Use capacity as initial max search value

                    // Step 3: Find Migration Target (least utilized between T_D and T_U)
                    for (int j = 0; j < MAX_AMFS; j++) {
                        AMF *candidate = &amfs[j];
                        if (!candidate->active || candidate == amf_old) continue;

                        float cand_util = (float)candidate->connections / AMF_CAPACITY;
                        if (cand_util >= THRESHOLD_DOWN && cand_util < THRESHOLD_UP) {
                            if (candidate->connections < min_load) {
                                min_load = candidate->connections;
                                amf_new = candidate;
                            }
                        }
                    }

                    // Step 4: Execute Migration
                    if (amf_new != NULL) {
                        printf("!!! SCALE DOWN: AMF %d (Util: %.2f) below T_D. Migrating to AMF %d (Load: %d)...\n",
                               amf_old->id, utilization, amf_new->id, amf_new->connections);

                        // 4a. Lock AMFs (Lock order to prevent deadlock: lower ID first)
                        if (amf_old->id < amf_new->id) {
                            pthread_mutex_lock(amf_old->lock);
                            pthread_mutex_lock(amf_new->lock);
                        } else {
                            pthread_mutex_lock(amf_new->lock);
                            pthread_mutex_lock(amf_old->lock);
                        }

                        // ** SIMULATE MIGRATION - PLACEHOLDER **
                        // In a real system, existing connection threads must be terminated
                        // and new forwarding threads to the new AMF must be established.

                        // 4b. & 4c. Update Load (stateless migration allows simple load transfer)
                        amf_new->connections += amf_old->connections;
                        amf_old->connections = 0;

                        // 4d. De-deploy AMF
                        amf_old->active = 0;
                        printf("De-scaling AMF %d. New load on AMF %d: %d\n",
                               amf_old->id, amf_new->id, amf_new->connections);

                        char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment",
                                       (i == 1) ? "core5g-amf-2-deployment" : "core5g-amf-3-deployment",
                                       "--replicas=0", NULL};
                        execute_command("kubectl", cmd);

                        // 4e. Unlock AMFs
                        pthread_mutex_unlock(amf_old->lock);
                        pthread_mutex_unlock(amf_new->lock);

                        // Break out of the outer loop after a successful de-scale
                        break;
                    }
                }
            }
        }
        pthread_mutex_unlock(&amf_state_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // ... (Signal handling and file setup omitted for brevity) ...

    if (argc < 2) {
        printf("Usage: %s <amf_capacity>\n", argv[0]);
        return 1;
    }

    fp = fopen("auto-scaling-latency.txt", "w");
    fp_amf = fopen("auto-scaling-amf.txt", "w");

    signal(SIGINT, handle_sigint);

    int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = inet_addr("10.0.3.1");
    listen_addr.sin_port = htons(38412);

    bind(listen_socket, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    listen(listen_socket, MAX_CONNECTIONS);

    printf("Proxy listening on IP 10.0.3.1, port 38412\n");
    printf("Consistent hashing ring: AMF1 at position 0, AMF2 at position 30, AMF3 at position 60\n");

    // Start health check thread
    pthread_create(&health_thread, NULL, health_check_thread, NULL);
    pthread_detach(health_thread);

    AMF_CAPACITY = atoi(argv[1]);

    // Initialize per-AMF mutex locks
    for (int i = 0; i < MAX_AMFS; i++) {
        amf_locks[i] = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(amf_locks[i], NULL);
        amfs[i].lock = amf_locks[i];
    }

    // ... (Socket setup and listen similar to original) ...
    // ... (Proxy listening print) ...

    pthread_t descaling_t;
    pthread_create(&descaling_t, NULL, descaling_thread, NULL);
    pthread_detach(descaling_t);
    printf("Descaling thread started (interval: %d minutes)\n", DESCALING_INTERVAL_MINUTES);

    while (1) {
        // ... (Accept connection from gNB) ...
        socklen_t addr_len = sizeof(struct sockaddr_in);
        int *gnb_socket = malloc(sizeof(int));
        *gnb_socket = accept(listen_socket, (struct sockaddr *)&gnb_addr, &addr_len);

        // 1. Check for Scale Up on every new connection
        scale_up_check();

        // 2. Handle the new connection
        pthread_t gnb_thread;
        pthread_create(&gnb_thread, NULL, handle_gnb_connection, gnb_socket);
        pthread_detach(gnb_thread);
    }

    // ... (Cleanup) ...
    return 0;
}