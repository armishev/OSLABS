#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

int main(){
    int fd1[2];
    if (pipe(fd1)==-1){
        perror("error pipe1");
    }
    int fd2[2];
    if (pipe(fd2)==-1){
        perror("error pipe2");
    }
    int fd3[2];
    if (pipe(fd3)==-1){
        perror("error pipe3");
    }
    int id = fork();
    if (id == -1)
    {
        perror("fork error");
        return 0;
    }else if(id == 0) {
        close(fd1[1]);
        close(fd2[0]);
        close(fd3[0]);
        close(fd3[1]);
        dup2(fd1[0], STDIN_FILENO);
        dup2(fd2[1], STDOUT_FILENO);
        execlp("./child1", "child1", NULL);
        close(fd1[0]);
        close(fd2[1]);
    }
    int id2;
    if(id > 0) {
        id2 = fork();
        if (id2 == -1)
        {
            perror("fork2 error");
            return 0;
        }else if(id2 == 0) {
            close(fd3[0]);
            close(fd2[1]);
            close(fd1[0]);
            close(fd1[1]);
            dup2(fd2[0], STDIN_FILENO);
            dup2(fd3[1], STDOUT_FILENO);
            execlp("./child2", "child2", NULL);
            close(fd2[0]);
            close(fd3[1]);
        }
    }
    if(id != 0 && id2 != 0) {
        close(fd1[0]);
        close(fd2[0]);
        close(fd2[1]);
        close(fd3[1]);
        char c[100];
        while(fgets(c,sizeof(c),stdin)) {
            int n = strlen(c);
            write(fd1[1], &c, n);
            int n1= read(fd3[0], &c, n);
            write(1,&c,n1);
        }
        close(fd3[0]);
        close(fd1[1]);
    }

    return 0;
}
