#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {
    int A_C[2];
    if (pipe(A_C)==-1){
        perror("error pipe1");
    }
    int A_B[2];
    if (pipe(A_B)==-1){
        perror("error pipe2");
    }
    int C_B[2];
    if (pipe(C_B)==-1){
        perror("error pipe3");
    }
    int C_A[2];
    if (pipe(C_A)==-1){
        perror("error pipe3");
    }
    int id = fork();
    if (id == -1)
    {
        perror("fork error");
        return 0;
    }else if(id == 0) {
        close(A_C[1]);  close(A_C[0]);  close(C_B[1]);  close(A_B[1]);  close(C_A[1]);  close(C_A[0]);
        char A_read[10];
        char C_read[10];
        sprintf(A_read, "%d", A_B[0]);
        sprintf(C_read, "%d", C_B[0]);
        char *B_argv[] = {"b", A_read, C_read, NULL};
        execv("b", B_argv);
    }
    int id2;
    if(id > 0) {
        id2 = fork();
        if (id2 == -1)
        {
            perror("fork2 error");
            return 0;
        }else if(id2 == 0) {
            close(A_C[1]); close(C_B[0]); close(A_B[1]); close(C_A[0]); close(A_B[0]);
            char A_read[10];
            char A_write[10];
            char B_write[10];
            sprintf(A_read, "%d", A_C[0]);
            sprintf(A_write, "%d", C_A[1]);
            sprintf(B_write, "%d", C_B[1]);

            char *C_argv[] = {"c", A_read, A_write, B_write, NULL};

            execv("c", C_argv);
        }
    }
    if(id != 0 && id2 != 0) {
        close(A_C[0]); close(A_B[0]); close(C_B[0]); close(C_B[1]); close(C_A[1]);
        char string[100];
        while (1) {
            if(gets(string) == NULL) {
                perror("gets error");
                return 0;
            }
            int length = strlen(string);

            if(length == 0) {
                if(write(A_C[1], &length, sizeof(length)) == -1) {
                    perror("Write in A_C error");
                    return 0;
                }

                if(write(A_B[1], &length, sizeof(length)) == -1) {
                    perror("Write in A_B error");
                    return 0;
                }
                break;
            }
            if(write(A_C[1], &length, sizeof(length)) == -1) {
                perror("Write in A_C error");
                return 0;
            }
            if(write(A_C[1], &string, sizeof(char) * length) == -1) {
                perror("Write in A_C error");
                return 0;
            }

            if(write(A_B[1], &length, sizeof(length)) == -1) {
                perror("Write in A_B error");
                return 0;
            }

            int check = 0;
            if(read(C_A[0], &check, sizeof(check)) == -1) {
                perror("Read from C_A error");
                return 1;
            }
        }
    }
    return 0;
}
