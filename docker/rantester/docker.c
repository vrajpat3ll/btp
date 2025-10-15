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
    //char *cmd1 = "sudo";
    char *argv1[] = {"sudo", "docker", "login", NULL};

    // Command 2: sudo docker build -t wriddhiraaj/my5g-ran-tester:latest .
    //char *cmd2 = "sudo";
    char *argv2[] = {"sudo", "docker", "build", "-t", "wriddhiraaj/my5g-ran-tester:latest", ".", NULL};

    // Command 3: sudo docker push wriddhiraaj/my5g-ran-tester:latest
    //char *cmd3 = "sudo";
    char *argv3[] = {"sudo", "docker", "push", "wriddhiraaj/my5g-ran-tester:latest", NULL};

    // Execute the commands
    
    execute(argv1);
    
    
    execute(argv2);
    
    execute(argv3);

    return 0;
}

