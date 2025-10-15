#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10
#define HASH_SPACE_SIZE 90

pthread_mutex_t gnb_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int gnb_count = 0;
int load = 2;
struct timeval start, end;

FILE *fp, *fp_amf;

// Store AMF information
typedef struct {
    unsigned int id;                 // AMF ID (1, 2, or 3)
    char ip[16];            // IP address
    unsigned int port;               // Port number
    unsigned int position;           // Position in the hash ring (0-89)
    int active;             // Whether the AMF is active (1) or down (0)
} AMF;
AMF amfs[3] = {
    {1, "10.0.3.3", 38412, 0, 1},   // AMF1 at position 0
    {2, "10.0.3.4", 38412, 30, 0},  // AMF2 at position 30
    {3, "10.0.3.5", 38412, 60, 0}   // AMF3 at position 60
};

void handle_sigint(int sig) {
    if (fp != NULL) {
        fclose(fp);
    }
    if (fp_amf != NULL) {
        fclose(fp_amf);
    }
    exit(0);
}

typedef struct {
    int source_socket;
    int destination_socket;
} forward_info_t;

// Simple hash function for UE IP addresses
// Takes the last octet of the IP and maps it to 0-89
int hash_ue_ip(const char *ip_str) {
    struct in_addr addr;
    inet_aton(ip_str, &addr);
    unsigned char *ip_bytes = (unsigned char *)&addr.s_addr;
    
    // Use the last octet as a simple hash
    return (ip_bytes[3] % HASH_SPACE_SIZE);
}

// Find the appropriate AMF for a given hash position
AMF* find_amf_for_position(int position) {
    // Go clockwise around the ring to find the first active AMF
    for (int i = 0; i < HASH_SPACE_SIZE; i++) {
        int current_pos = (position + i) % HASH_SPACE_SIZE;
        
        // Check if any AMF is at this position
        for (int j = 0; j < 3; j++) {
            if (amfs[j].active && amfs[j].position == current_pos) {
                return &amfs[j];
            }
        }
    }
    
    // Fallback: return the first active AMF
    for (int j = 0; j < 3; j++) {
        if (amfs[j].active) {
            return &amfs[j];
        }
    }
    
    // If no active AMFs, return NULL
    return NULL;
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

// Function to forward messages from source to destination
void *forward_messages(void *arg) {
    forward_info_t *info = (forward_info_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;

    while ((nbytes = sctp_recvmsg(info->source_socket, buffer, sizeof(buffer), NULL, 0, NULL, NULL)) > 0) {
        sctp_sendmsg(info->destination_socket, buffer, nbytes, NULL, 0, 0, 0, 0, 0, 0);
    }

    close(info->source_socket);
    close(info->destination_socket);
    free(info);
    return NULL;
}

// Function to handle each gNB connection
void *handle_gnb_connection(void *arg)
{
    gettimeofday(&start, NULL);
    int gnb_socket = *(int *)arg;
    free(arg);

    // Get gNB (client) IP address
    struct sockaddr_in gnb_addr;
    socklen_t addr_len = sizeof(gnb_addr);
    getpeername(gnb_socket, (struct sockaddr *)&gnb_addr, &addr_len);
    char gnb_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &gnb_addr.sin_addr, gnb_ip, INET_ADDRSTRLEN);

    // Hash the gNB IP to find its position in the hash space
    int hash_position = hash_ue_ip(gnb_ip);
    printf("gNB IP %s hashed to position %d\n", gnb_ip, hash_position);

    // Find the appropriate AMF using consistent hashing
    AMF *target_amf = find_amf_for_position(hash_position);
    if (target_amf == NULL) {
        printf("No active AMFs available!\n");
        close(gnb_socket);
        return NULL;
    }

    int amf_socket;
    struct sockaddr_in amf_addr;
    pthread_t gnb_to_amf_thread, amf_to_gnb_thread;
    forward_info_t *gnb_to_amf_info, *amf_to_gnb_info;

    // Connect to the selected AMF
    amf_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    memset(&amf_addr, 0, sizeof(amf_addr));
    amf_addr.sin_family = AF_INET;
    amf_addr.sin_addr.s_addr = inet_addr(target_amf->ip);
    amf_addr.sin_port = htons(target_amf->port);

    // Try to connect to the selected AMF
    int connect_status = connect(amf_socket, (struct sockaddr *)&amf_addr, sizeof(amf_addr));
    
    // If connection fails, mark this AMF as inactive and try to find another one
    if (connect_status < 0) {
        printf("AMF %d at %s is down. Marking as inactive.\n", target_amf->id, target_amf->ip);
        target_amf->active = 0;
        
        // Find another AMF
        target_amf = find_amf_for_position(hash_position);
        if (target_amf == NULL) {
            printf("No active AMFs available after failover!\n");
            close(gnb_socket);
            return NULL;
        }
        
        // Try connecting to the fallback AMF
        amf_addr.sin_addr.s_addr = inet_addr(target_amf->ip);
        connect_status = connect(amf_socket, (struct sockaddr *)&amf_addr, sizeof(amf_addr));
        
        if (connect_status < 0) {
            printf("Fallback AMF %d is also down!\n", target_amf->id);
            close(gnb_socket);
            close(amf_socket);
            return NULL;
        }
    }

    gettimeofday(&end, NULL);
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    printf("Connected to AMF %d in %.2f ms\n", target_amf->id, elapsed_time);
    fprintf(fp, "%.2f\n", elapsed_time);
    fprintf(fp_amf, "%d\n", target_amf->id);
    fflush(fp_amf);

    // Create threads to handle message forwarding
    gnb_to_amf_info = malloc(sizeof(forward_info_t));
    gnb_to_amf_info->source_socket = gnb_socket;
    gnb_to_amf_info->destination_socket = amf_socket;
    pthread_create(&gnb_to_amf_thread, NULL, forward_messages, gnb_to_amf_info);

    amf_to_gnb_info = malloc(sizeof(forward_info_t));
    amf_to_gnb_info->source_socket = amf_socket;
    amf_to_gnb_info->destination_socket = gnb_socket;
    pthread_create(&amf_to_gnb_thread, NULL, forward_messages, amf_to_gnb_info);

    // Wait for both threads to finish
    pthread_join(gnb_to_amf_thread, NULL);
    pthread_join(amf_to_gnb_thread, NULL);

    close(gnb_socket);
    close(amf_socket);
    return NULL;
}

// Function to periodically check AMF health
void *health_check_thread(void *arg) {
    while (1) {
        for (int i = 0; i < 3; i++) {
            if (!amfs[i].active) {
                // Try to reconnect to inactive AMFs
                int check_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
                struct sockaddr_in check_addr;
                memset(&check_addr, 0, sizeof(check_addr));
                check_addr.sin_family = AF_INET;
                check_addr.sin_addr.s_addr = inet_addr(amfs[i].ip);
                check_addr.sin_port = htons(amfs[i].port);
                
                // Attempt connection with timeout
                struct timeval tv;
                tv.tv_sec = 1; // 1 second timeout
                tv.tv_usec = 0;
                setsockopt(check_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                
                if (connect(check_socket, (struct sockaddr *)&check_addr, sizeof(check_addr)) >= 0) {
                    printf("AMF %d at %s is back online!\n", amfs[i].id, amfs[i].ip);
                    amfs[i].active = 1;
                }
                
                close(check_socket);
            }
        }
        sleep(5); // Check every 5 seconds
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int listen_socket;
    struct sockaddr_in listen_addr, gnb_addr;
    pthread_t gnb_thread, health_thread;

    if (argc < 2) {
        printf("Usage: %s <amf_capacity>\n", argv[0]);
        return 0;
    }

    fp = fopen("consistent_hash_latency.txt", "w");
    fp_amf = fopen("consistent_hash_amf.txt", "w");

    signal(SIGINT, handle_sigint);

    load = atoi(argv[1]);
    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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

    while (1) {
        // Accept connection from gNB
        socklen_t addr_len = sizeof(gnb_addr);
        int *gnb_socket = malloc(sizeof(int));
        *gnb_socket = accept(listen_socket, (struct sockaddr *)&gnb_addr, &addr_len);
        
        // Get gNB IP for logging
        char gnb_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &gnb_addr.sin_addr, gnb_ip, INET_ADDRSTRLEN);
        
        pthread_mutex_lock(&gnb_count_mutex);
        gnb_count++;
        int current_gnb = gnb_count;
        pthread_mutex_unlock(&gnb_count_mutex);
        
        printf("\ngNB %d connected from IP %s\n", current_gnb, gnb_ip);

        // Create a new thread to handle the gNB connection
        pthread_create(&gnb_thread, NULL, handle_gnb_connection, gnb_socket);
        pthread_detach(gnb_thread);

        // Check if it's time to scale up the deployments (we keep this logic from original)
        if (current_gnb == load) {
            char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment", "core5g-amf-2-deployment", "--replicas=1", NULL};
            execute_command("kubectl", cmd);
        } else if (current_gnb == 2 * load) {
            char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment", "core5g-amf-3-deployment", "--replicas=1", NULL};
            execute_command("kubectl", cmd);
        }
    }

    close(listen_socket);
    return 0;
}
