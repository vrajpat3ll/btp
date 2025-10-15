#include "../include/utils.h"

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

void execute_command(const char *cmd, char *const args[])
{
    pid_t pid = fork();

    if (pid == -1) {
        log_perror("fork");
        return; // don't exit entire program from helper
    } else if (pid == 0) {
        // Child process
        if (execvp(cmd, args) == -1) {
            log_perror("execvp");
            _exit(EXIT_FAILURE);
        }
    } else {
        // Parent process: wait for child
        int status = 0;
        if (waitpid(pid, &status, 0) == -1) {
            log_perror("waitpid");
        }
    }
}

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *log_file = NULL;

int log_init(const char *filename) {
    pthread_mutex_lock(&log_mutex);
    log_file = fopen(filename, "a");
    if (!log_file)
    {
        log_perror("[log_init] fopen");
        pthread_mutex_unlock(&log_mutex);
        return -1;
    }
    pthread_mutex_unlock(&log_mutex);
    return 0;
}

void log_close() {
    pthread_mutex_lock(&log_mutex);
    if (log_file)
        fclose(log_file);
    log_file = NULL;
    pthread_mutex_unlock(&log_mutex);
}

void log(const char *level, const char *format, ...) {
    pthread_mutex_lock(&log_mutex);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    const char *color;
    if (strcmp(level, "INFO") == 0)
        color = COLOR_CYAN;
    else if (strcmp(level, "DEBUG") == 0)
        color = COLOR_BLUE;
    else if (strcmp(level, "WARN") == 0)
        color = COLOR_YELLOW;
    else if (strcmp(level, "ERROR") == 0)
        color = COLOR_RED;
    else if (strcmp(level, "SUCCESS") == 0)
        color = COLOR_GREEN;
    else
        color = COLOR_MAGENTA;

    char msg[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    printf("%s[%04d-%02d-%02d %02d:%02d:%02d] [%s]%s %s",
           color,
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec,
           level,
           COLOR_RESET,
           msg);

    if (log_file)
    {
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec,
                level,
                msg);
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_mutex);
}

void log_perror(const char *message) {
    pthread_mutex_lock(&log_mutex);

    int errnum = errno;  // Save errno right away

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    const char *color, *level="ERROR";
    if (strcmp(level, "INFO") == 0) color = COLOR_CYAN;
    else if (strcmp(level, "DEBUG") == 0) color = COLOR_BLUE;
    else if (strcmp(level, "WARN") == 0) color = COLOR_YELLOW;
    else if (strcmp(level, "ERROR") == 0) color = COLOR_RED;
    else if (strcmp(level, "SUCCESS") == 0) color = COLOR_GREEN;
    else color = COLOR_MAGENTA;

    const char *err_str = strerror(errnum);

    fprintf(stderr, "%s[%04d-%02d-%02d %02d:%02d:%02d] [%s]%s %s: %s\n",
            color,
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            level,
            COLOR_RESET,
            message,
            err_str);

    if (log_file) {
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s: %s\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec,
                level,
                message,
                err_str);
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_mutex);
}

// int main() {
//     log("INFO", "This is an info message.");
//     log("DEBUG", "This is a debug message.");
//     log("WARN", "This is a warning message.");
//     log("ERROR", "This is an error message.");
//     // usleep(1000000);
//     log("SUCCESS", "This is a success message.");
//     log("CUSTOM", "This is a custom level message.");

//     return 0;
// }