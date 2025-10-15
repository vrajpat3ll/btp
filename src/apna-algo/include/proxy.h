#ifndef PROXY_H
#define PROXY_H

#include "amf.h"

#define BUFFER_SIZE 1024
extern int total_conn_count;

// Structures
typedef struct {
    int source_socket;
    int destination_socket;
    AMF *source_amf;
    AMF *destination_amf;
} forward_info_t;

// Functions
void *handle_gnb_connection(void *arg);
void *forward_messages(void *arg);
void execute_command(char *cmd, char *args[]);

#endif
