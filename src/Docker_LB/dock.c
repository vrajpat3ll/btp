#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void execute(char *const argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (execvp(argv[0], argv) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("%s exited with status %d\n", argv[0], WEXITSTATUS(status));
        } else {
            printf("%s did not exit successfully\n", argv[0]);
        }
    }
}

int main() {
    // Command 1: sudo docker login
    char *const login_cmd[] = {"sudo", "docker", "login", NULL};
    execute(login_cmd);

    // Command 2: sudo docker build -t wriddhiraaj/5g_loadbalancer .
    char *const build_cmd[] = {"sudo", "docker", "build", "-t", "wriddhiraaj/5g_loadbalancer", ".", NULL};
    execute(build_cmd);

    // Command 3: sudo docker tag wriddhiraaj/5g_loadbalancer wriddhiraaj/5g_loadbalancer:v1.0.0
    char *const tag_cmd[] = {"sudo", "docker", "tag", "wriddhiraaj/5g_loadbalancer", "wriddhiraaj/5g_loadbalancer:v1.0.0", NULL};
    execute(tag_cmd);

    // Command 4: sudo docker push wriddhiraaj/5g_loadbalancer:v1.0.0
    char *const push_cmd[] = {"sudo", "docker", "push", "wriddhiraaj/5g_loadbalancer:v1.0.0", NULL};
    execute(push_cmd);

    return 0;
}

