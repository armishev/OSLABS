#include "stdio.h"
#include "stdlib.h"
#include "math1.h"
#include "math2.h"

int main(int argc, char *argv[]){
    // Argument error check
    if (argc == 1){
        perror("INCORRECT INPUT KEYS\n");
        exit(0);
    }
    else {
        if ((atoi(argv[1]) == 1)) {
            printf("%f\n", SinIntegral1(atof(argv[2]), atof(argv[3]), atof(argv[4])));
        }
        else if((atoi(argv[1]) == 2)){
            printf("%f\n", derivative1(atof(argv[2]), atof(argv[3])));
        }
        return 0;
    }
}