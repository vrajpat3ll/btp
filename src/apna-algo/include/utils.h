#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"

void execute_command(const char *cmd, char *const args[]);

int log_init(const char *filename);
void log_close();
void log(const char *level, const char *format, ...);
void log_perror(const char *message);

#endif // UTILS_H