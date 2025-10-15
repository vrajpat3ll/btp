#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>

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

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_tabs>\n", argv[0]);
        return 1;
    }
    
    

    int gnb = atoi(argv[1]);
    
    //int amf_load = ceil(gnb/3.0);
    
    
    
    
    FILE* fp = fopen("time.txt","w");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    srand(time(NULL));  // Seed the random number generator with the current time

    int i;
    char command[1024];
    
    int s[gnb];
    
    int sum = 0;

    for (i = 1; i <= gnb; i++) 
    {
        s[i-1] = sum;
        fprintf(fp,"%d\n",sum);
    
        int r = rand() % 4 + 1;
    
        snprintf(command, sizeof(command), 
                 "gnome-terminal --tab -- bash -c 'sudo kubectl -n ran-simulator%d exec deploy/sim5g-simulator -- /root/go/src/my5G-RANTester/cmd/app ue; exec bash'", 
                 i);
        int result = system(command);
        
        if (result == -1) {
            perror("Error executing command");
            fclose(fp);
            return 1;
        }
        
        sleep(r);
        
        sum += r;
    }

    fclose(fp);

    // Create and write the Python script
    FILE* py_fp = fopen("plot_amfs.py", "w");
    if (py_fp == NULL) {
        perror("Error opening Python file");
        return 1;
    }

    fprintf(py_fp, "import matplotlib.pyplot as plt\n\n");

    // Write x values
    fprintf(py_fp, "x = [");
    for (i = 0; i < gnb; i++) {
        fprintf(py_fp, "%d", s[i]);
        if (i < gnb - 1) {
            fprintf(py_fp, ", ");
        }
    }
    fprintf(py_fp, "]\n");
    
    
    

    // Write y values
    
    if(gnb<=30)
    fprintf(py_fp, "y = [1] * %d + 1\n\n",gnb);
    else if(gnb>30 && gnb<=60)
    fprintf(py_fp, "y = [1] * 30 + [2] * %d \n\n",(gnb-30));
    else
    fprintf(py_fp, "y = [1] * 30 + [2] * 30 + [3] * %d\n\n",(gnb-60));

    fprintf(py_fp, "plt.figure(figsize=(10, 5))\n");
    fprintf(py_fp, "plt.step(x, y, where='post', linestyle='-', color='b', marker='o')\n");
    fprintf(py_fp, "plt.xlabel('Time (seconds)')\n");
    fprintf(py_fp, "plt.ylabel('Number of AMFs')\n");
    fprintf(py_fp, "plt.title('Number of AMFs over Time')\n");
    fprintf(py_fp, "plt.grid(True)\n");

    // Annotating the transitions
    if(gnb<=60)
    {
    	if(gnb<=30)
    	fprintf(py_fp, "plt.text(%d, 1.5, 'gnb_count = %d', horizontalalignment='center')\n",(s[gnb-1]+25),gnb);
    	else
    	fprintf(py_fp, "plt.text(%d, 1.5, 'gnb_count = %d', horizontalalignment='center')\n",(s[30]+25),31);
    }	
    else if(gnb > 60)
    {
    	fprintf(py_fp, "plt.text(%d, 2.5, 'gnb_count = 61', horizontalalignment='center')\n",s[60]+25);
    	fprintf(py_fp, "plt.text(%d, 1.5, 'gnb_count = 31', horizontalalignment='center')\n",s[30]+25);
    }	

    // Setting y-axis ticks to only show 1, 2, 3
    fprintf(py_fp, "plt.yticks([1, 2, 3])\n");

    // Save the figure
    fprintf(py_fp, "plt.savefig('reactive_amf_time.png')\n");

    fclose(py_fp);

    // Execute the Python script
    snprintf(command, sizeof(command), "python3 plot_amfs.py");
    int result = system(command);
    if (result == -1) {
        perror("Error executing Python script");
        return 1;
    }
    
    	
    return 0;
}

