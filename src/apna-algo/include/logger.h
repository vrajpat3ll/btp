#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define LOG_QUEUE_SIZE 4096    // power of 2 for ring buffer
#define MAX_LOG_MSG_LEN 512
#define LOG_BATCH_SIZE 64      // how many messages to write at once

typedef struct {
    char messages[LOG_QUEUE_SIZE][MAX_LOG_MSG_LEN];
    int head;
    int tail;
    bool done;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    FILE *file;
    pthread_t thread;
} AsyncLogger;

// Logger thread function
void* logger_thread(void *arg) {
    AsyncLogger *logger = (AsyncLogger*)arg;
    char batch_buffer[LOG_BATCH_SIZE][MAX_LOG_MSG_LEN];
    int count = 0;

    while (true) {
        pthread_mutex_lock(&logger->lock);
        while (logger->head == logger->tail && !logger->done)
            pthread_cond_wait(&logger->cond, &logger->lock);

        if (logger->done && logger->head == logger->tail) {
            pthread_mutex_unlock(&logger->lock);
            break;
        }

        // Collect up to LOG_BATCH_SIZE messages
        count = 0;
        while (logger->tail != logger->head && count < LOG_BATCH_SIZE) {
            strcpy(batch_buffer[count], logger->messages[logger->tail]);
            logger->tail = (logger->tail + 1) & (LOG_QUEUE_SIZE - 1);
            count++;
        }
        pthread_mutex_unlock(&logger->lock);

        // Write the batch in one go
        for (int i = 0; i < count; ++i) {
            fputs(batch_buffer[i], logger->file);
            fputc('\n', logger->file);
        }

        // Flush less frequently for better performance
        fflush(logger->file);
    }

    return NULL;
}

// Initialize async logger
void async_logger_init(AsyncLogger *logger, const char *filename) {
    memset(logger, 0, sizeof(*logger));
    pthread_mutex_init(&logger->lock, NULL);
    pthread_cond_init(&logger->cond, NULL);
    logger->file = fopen(filename, "a");
    if (!logger->file) {
        perror("fopen");
        exit(1);
    }
    pthread_create(&logger->thread, NULL, logger_thread, logger);
}

// Enqueue a log message
void async_log(AsyncLogger *logger, const char *msg) {
    pthread_mutex_lock(&logger->lock);

    int next_head = (logger->head + 1) & (LOG_QUEUE_SIZE - 1);
    if (next_head == logger->tail) {
        // Queue full: drop message
        pthread_mutex_unlock(&logger->lock);
        return;
    }

    snprintf(logger->messages[logger->head], MAX_LOG_MSG_LEN, "%s", msg);
    logger->head = next_head;
    pthread_cond_signal(&logger->cond);
    pthread_mutex_unlock(&logger->lock);
}

// Graceful shutdown
void async_logger_stop(AsyncLogger *logger) {
    pthread_mutex_lock(&logger->lock);
    logger->done = true;
    pthread_cond_signal(&logger->cond);
    pthread_mutex_unlock(&logger->lock);

    pthread_join(logger->thread, NULL);
    fclose(logger->file);
    pthread_mutex_destroy(&logger->lock);
    pthread_cond_destroy(&logger->cond);
}

// Example worker function
void* worker_func(void* arg) {
    AsyncLogger* logger = (AsyncLogger*)arg;
    char buf[128];
    for (int i = 0; i < 5000; ++i) {
        snprintf(buf, sizeof(buf), "Worker %lu iteration %d", pthread_self(), i);
        async_log(logger, buf);
        usleep(200); // simulate work
    }
    return NULL;
}

// int main() {
//     AsyncLogger logger;
//     async_logger_init(&logger, "async_batched_log.txt");

//     pthread_t workers[4];
//     for (int i = 0; i < 4; ++i)
//         pthread_create(&workers[i], NULL, worker_func, &logger);

//     for (int i = 0; i < 4; ++i)
//         pthread_join(workers[i], NULL);

//     async_logger_stop(&logger);
//     return 0;
// }
