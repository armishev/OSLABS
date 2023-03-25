#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <sys/wait.h>

void *spaces(char* str, int size) {
    int i, x;
    for(i=x=0; i< size; ++i)
        if(!isspace(str[i]) || (i > 0 && !isspace(str[i-1])))
            str[x++] = str[i];
    str[x] = '\0';
    return str;
}

int main(){
    int FILE = open("./IN.txt",O_CREAT | O_RDWR,S_IRUSR | S_IWUSR);
    truncate("./IN.txt", 0);
    struct stat sb;
    char c[100];
    while(fgets(c,sizeof(c),stdin)) {
        int n = strlen(c);
        write(FILE, &c, n);
    }
    if(fstat(FILE,&sb)==-1){
        perror("couldn't get file size.\n");
    }
    char *file_in_memory = mmap(NULL, sb.st_size,PROT_READ | PROT_WRITE, MAP_SHARED, FILE,0);
    int id = fork();
    if (id == -1)
    {
        perror("fork error");
        return 0;
    }else if(id == 0) {
        for(int i=0; i<sb.st_size; i++){
            file_in_memory[i]=toupper(file_in_memory[i]);
        }
        return 0;
    }
    int id2;
    if(id > 0) {
        id2 = fork();
        if (id2 == -1)
        {
            perror("fork2 error");
            return 0;
        }else if(id2 == 0) {
            spaces(file_in_memory,sb.st_size);
        }
    }
    if(id != 0 && id2 != 0) {
        wait(NULL);
        wait(NULL);
        for(int i=0; file_in_memory[i]; ++i){
            printf("%c", file_in_memory[i]);
        }
        printf("\n");
        munmap(file_in_memory, sb.st_size);
    }

    return 0;
}

