#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10

pthread_mutex_t gnb_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int gnb_count = 1;
int load = 2;
struct timeval start, end;

FILE* fp;

void handle_sigint(int sig) {
    if (fp != NULL) {
        fclose(fp);
    }
    exit(0);
}

typedef struct {
    int source_socket;
    int destination_socket;
} forward_info_t;

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

    int amf_socket;
    struct sockaddr_in amf_addr;
    pthread_t gnb_to_amf_thread, amf_to_gnb_thread;
    forward_info_t *gnb_to_amf_info, *amf_to_gnb_info;

    pthread_mutex_lock(&gnb_count_mutex);
    gnb_count++;
    if(gnb_count > 3*load)
    gnb_count = 1;
    int current_gnb_count = gnb_count;
    pthread_mutex_unlock(&gnb_count_mutex);

    // Connect to AMF based on gNB count
    amf_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

    memset(&amf_addr, 0, sizeof(amf_addr));
    amf_addr.sin_family = AF_INET;

    if (current_gnb_count <= load) {
        amf_addr.sin_addr.s_addr = inet_addr("10.0.3.3");
        amf_addr.sin_port = htons(38412);
    } else if (current_gnb_count > load && current_gnb_count <= 2 * load) {
        amf_addr.sin_addr.s_addr = inet_addr("10.0.3.4");
        amf_addr.sin_port = htons(38412);
    } else if (current_gnb_count > 2 * load) {
        amf_addr.sin_addr.s_addr = inet_addr("10.0.3.5");
        amf_addr.sin_port = htons(38412);
    }

    while (connect(amf_socket, (struct sockaddr *)&amf_addr, sizeof(amf_addr)) < 0) {
       /* perror("connect");
        sleep(1);*/
    }
    gettimeofday(&end, NULL);
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    printf("Connected to AMF in %.2f ms\n", elapsed_time);
    fprintf(fp,"%.2f\n",elapsed_time);
    //printf("Connected to AMF\n");

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

int main(int argc, char *argv[]) {
    int listen_socket;
    struct sockaddr_in listen_addr, gnb_addr;
    pthread_t gnb_thread;

    if (argc < 2) {
        printf("Usage: %s <amf_capacity>\n", argv[0]);
        return 0;
    }

    fp = fopen("proactive_latency.txt", "w");
    //fp_amf = fopen("amf.txt", "w");

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

    while (1) {
        // Accept connection from gNB
        socklen_t addr_len = sizeof(gnb_addr);
        int *gnb_socket = malloc(sizeof(int));
        *gnb_socket = accept(listen_socket, (struct sockaddr *)&gnb_addr, &addr_len);
        gettimeofday(&start, NULL);

        // Create a new thread to handle the gNB connection
        pthread_create(&gnb_thread, NULL, handle_gnb_connection, gnb_socket);
        pthread_detach(gnb_thread);

        // Check if it's time to scale up the deployments
        pthread_mutex_lock(&gnb_count_mutex);
        int count = gnb_count;
        pthread_mutex_unlock(&gnb_count_mutex);

        //count = count + 1;

        printf("\nGNB %d connected\n\n", count);

        if (count == load) {
            char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment", "core5g-amf-2-deployment", "--replicas=1", NULL};
            execute_command("kubectl", cmd);
        } else if (count == 2 * load) {
            char *cmd[] = {"kubectl", "-n", "open5gs", "scale", "deployment", "core5g-amf-3-deployment", "--replicas=1", NULL};
            execute_command("kubectl", cmd);
        }
    }

    close(listen_socket);
    return 0;
}


