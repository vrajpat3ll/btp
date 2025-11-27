#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "amf.h"
#include "forward.h"
#include "scale.h"
#include "utils.h"

int listen_socket;
FILE* latency_file;
char latency_log_filename[256];
char migration_log_filename[256];

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // logs setup
    {
        struct stat st = {0};

        // logs/ directory exists check
        if (stat("logs", &st) == -1) {
            if (mkdir("logs", 0755) != 0) {
                perror("[main] mkdir logs");
            }
        }

        // Timestamp
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        char ts[32];  // timestamp string: YYYYMMDD-HHMMSS
        if (strftime(ts, sizeof(ts), "%Y-%m-%d_%H%M%S", &tm) == 0) {
            // Fallback: just use epoch time
            snprintf(ts, sizeof(ts), "%ld", (long)t);
        }

        // logs/<timestamp> directory
        char dir[256];
        snprintf(dir, sizeof(dir), "logs/%s", ts);

        if (stat(dir, &st) == -1) {
            if (mkdir(dir, 0755) != 0) {
                perror("[main] mkdir timestamped logs dir");
            }
        }

        // run.log path: logs/{timestamp}/run.log
        char fname[256];
        snprintf(fname, sizeof(fname), "%s/run.log", dir);

        // associative-latency.log path: logs/{timestamp}/associative-latency.log
        snprintf(latency_log_filename, sizeof(latency_log_filename),
                 "%s/associative-latency.csv", dir);

        // migration log path: logs/{timestamp}/migration-log.csv
        snprintf(migration_log_filename, sizeof(migration_log_filename), "%s/migration-latency.csv", dir);

        if (log_init(fname) != 0) {
            fprintf(stderr, "[main] Failed to initialize log file %s\n", fname);
        } else {
            log("INFO", "[main] Logging initialized at %s\n", fname);
        }
        latency_file = fopen(latency_log_filename, "a");
        if (!latency_file) {
            log_perror("[main] Failed to open latency log file");
        } else {
            fprintf(latency_file, "timestamp,gnb_ip,amf_ip,latency_us,latency_ms\n");
            fflush(latency_file);
            log("INFO", "[main] Latency log initialized at %s\n", latency_log_filename);
        }
        // Correct fclose check: fclose returns 0 on success
        if (latency_file && fclose(latency_file) != 0) {
            log_perror("[main] Failed to close latency log file");
        }

        FILE* migration_file = fopen(migration_log_filename, "a");
        if (!migration_file) {
            log_perror("[main] Failed to open migration log file");
        } else {
            // CSV header: timestamp,gnb_ip,old_amf_ip,new_amf_ip,migration_us,migration_ms
            fprintf(migration_file, "timestamp,gnb_ip,old_amf_ip,new_amf_ip,migration_us,migration_ms\n");
            fflush(migration_file);
        }
        if (migration_file && fclose(migration_file) != 0) {
            log_perror("[main] Failed to close migration log file");
        }
    }

    // Write PID to current working directory ./logs/tmp.pid
    {
        char pidfile[512];
        snprintf(pidfile, sizeof(pidfile), "logs/tmp.pid");
        FILE* pf = fopen(pidfile, "w");
        if (pf) {
            fprintf(pf, "%d\n", (int)getpid());
            fclose(pf);
            log("INFO", "[main] Wrote PID %d to %s\n", (int)getpid(), pidfile);
        } else {
            log_perror("[main] Failed to open PID file for writing");
        }
    }

    amf_init_default();
    forward_init_table();
    log("DEBUG", "[main] Initialized tables...\n");

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listen_socket < 0) {
        log_perror("[main] socket");
        return 1;
    }

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = inet_addr(HOST_IP);
    listen_addr.sin_port = htons(PORT);

    log("DEBUG", "[main] Binding socket to %s:%d\n", HOST_IP, PORT);
    if (bind(listen_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        log_perror("[main] bind");
        close(listen_socket);
        return 1;
    }

    log("INFO", "[main] Setting up socket for listening (%d max connections)...\n", MAX_CONNECTIONS);
    if (listen(listen_socket, MAX_CONNECTIONS) < 0) {
        log_perror("[main] listen");
        close(listen_socket);
        return 1;
    }

    log("INFO", "[main] Proxy listening on %s:%d with AMF capacity %d\n", HOST_IP, PORT, AMF_CAPACITY);

    pthread_t descaling_t;
    if (pthread_create(&descaling_t, NULL, descaler, NULL) != 0) {
        log_perror("[main] pthread_create: descaler");
        close(listen_socket);
        return 1;
    }
    pthread_detach(descaling_t);

    while (1) {
        struct sockaddr_in gnb_addr;
        socklen_t addr_len = sizeof(gnb_addr);
        int* gnb_sock = malloc(sizeof(int));
        if (!gnb_sock) {
            log_perror("[main] malloc");
            continue;
        }
        *gnb_sock = accept(listen_socket, (struct sockaddr*)&gnb_addr, &addr_len);

        if (*gnb_sock < 0) {
            log_perror("[main] accept failed");
            free(gnb_sock);
            continue;
        }
        log("INFO", "[main] Requested gNB connection from %s:%d\n", inet_ntoa(gnb_addr.sin_addr), ntohs(gnb_addr.sin_port));

        scale_up();

        pthread_t gnb_thread;
        if (pthread_create(&gnb_thread, NULL, handle_gnb_connection, gnb_sock) != 0) {
            log_perror("[main] pthread_create: handle_gnb_connection");
            close(*gnb_sock);
            free(gnb_sock);
            continue;
        }
        pthread_detach(gnb_thread);
    }
    log("INFO", "[main] Closing listening socket and exiting.\n");
    log_close();

    close(listen_socket);
    return 0;
}