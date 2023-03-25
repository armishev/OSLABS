#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>



int main(int argc, const char *argv[]) {
    int length;
    int file_descriptors[2];
    file_descriptors[0] = atoi(argv[1]); //read from A
    file_descriptors[1] = atoi(argv[2]); //read from C

    while (1){
        if (read(file_descriptors[0], &length, sizeof(int)) == -1) {
            perror("Read from A error");
        }

        if(length == 0) {
            break;
        }

        printf("from A = %d\n", length);

        if(read(file_descriptors[1], &length, sizeof(int)) == -1) {
            perror("Read from C error");
            return 1;
        }
        printf("from C = %d\n", length);
    }

    return 0;
}