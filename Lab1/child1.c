#include<unistd.h>
#include <ctype.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    char *file_in_memory = argv[1];
    int size = atoi(argv[2]);
    for(int i=0; i<size; i++){
        file_in_memory[i]=toupper(  file_in_memory[i]);
    }
    return 0;
}