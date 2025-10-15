#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char**argv) {
    FILE *file;
    int i;
    int randomValue;

    // Seed the random number generator
    srand(time(NULL));

    // Open the file for writing
    file = fopen("data/ue-conn-timediff.txt", "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    // Generate random values and write them to the file
    for (i = 0; i < atoi(argv[1]); i++) { // Adjust the loop count for more values
        randomValue = (rand() % 5) + 1; // Generate random value between 1 and 5
        fprintf(file, "%d\n", randomValue);
    }

    // Close the file
    fclose(file);

    printf("Random values written to ue-conn-timediff.txt\n");

    return 0;
}

