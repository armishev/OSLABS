#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, const char *argv[]) {
    int file_descriptors[3];
    file_descriptors[0] = atoi(argv[1]);  //read from A
    file_descriptors[1] = atoi(argv[2]);  //write to A
    file_descriptors[2] = atoi(argv[3]);  //write to B

    int length;
    char string[100];
    memset(string, 0, 100);

    while (1) {
        if(read(file_descriptors[0], &length, sizeof(int)) == -1) {
            perror("Read from A error");
        }

        if(length == 0) {
            break;
        }

        if(read(file_descriptors[0], &string, sizeof(char) * length) == -1) {
            perror("Read from A error");
        }

        printf("C: %s\n", string);

        if(write(file_descriptors[1], &length, sizeof(int)) == -1) {
            perror("Write to A error");
        }

        if(write(file_descriptors[2], &length, sizeof(int)) == -1) {
            perror("Write to B error");
        }
        memset(string, 0, 100);
        length = 0;
    }
    return 0;
}