#include <ctype.h>
#include <stdlib.h>

void *spaces(char* str,int size) {
    int i, x;
    for(i=x=0; i<size; ++i)
        if(!isspace(str[i]) || (i > 0 && !isspace(str[i-1])))
            str[x++] = str[i];
    str[x] = '\0';
    return str;
}

int main(int argc, char* argv[]) {
    char *file_in_memory = argv[1];
    int size = atoi(argv[2]);
    spaces(file_in_memory,size);
    return 0;
}

