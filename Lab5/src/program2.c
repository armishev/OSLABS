#include <stdio.h>
#include <dlfcn.h>
#include <math.h>
#include <stdlib.h>
int main(int argc, char * argv[]) {
    void * handler1;
    void * handler2;
    float(*Integral)(float, float, float);
    float(*Derivative)(float, float);
    if (argc > 2) {
        if(atoi(argv[1]) == 0) {
            if(atoi(argv[2]) == 1) {
                handler1 = dlopen("/Users/kirillarmishev/Downloads/project/cmake-build-debug/src/libraries/libmath22.dylib", RTLD_LAZY);
                if (!handler1) {
                    fprintf(stderr, "Open of dynamic library math22 with error: %s\n", dlerror());
                    exit(-1);
                }
                Derivative = dlsym(handler1, "derivative2");
                printf("result of derivative(2 realisation)  is %f\n", (*Derivative)(atof(argv[3]), atof(argv[4])));
                dlclose(handler1);
            } else {
                handler2 = dlopen("/Users/kirillarmishev/Downloads/project/cmake-build-debug/src/libraries/libmath12.dylib", RTLD_LAZY);
                if (!handler2) {
                    fprintf(stderr, "Open of dynamic library math12 with error: %s\n", dlerror());
                    exit(-2);
                }
                Integral = dlsym(handler2, "SinIntegral2");
                printf("result of integral is %f\n", (*Integral)(atof(argv[3]),atof(argv[4]),atof(argv[5])));
                dlclose(handler2);
            }
        }
        else if(atoi(argv[1]) == 1) {
            handler1 = dlopen("/Users/kirillarmishev/Downloads/project/cmake-build-debug/src/libraries/libmath2.dylib", RTLD_LAZY);
            if (!handler1) {
                fprintf(stderr, "Open of dynamic library math2 with error: %s\n", dlerror());
                exit(-1);
            }
            Derivative = dlsym(handler1, "derivative1");
            printf("result of derivative(1 realisation) is %f\n", (*Derivative)(atof(argv[2]), atof(argv[3])));
            dlclose(handler1);
        }
        else if(atoi(argv[1]) == 2) {
            handler2 = dlopen("/Users/kirillarmishev/Downloads/project/cmake-build-debug/src/libraries/libmath1.dylib", RTLD_LAZY);
            if (!handler2) {
                fprintf(stderr, "Open of dynamic library math1 with error: %s\n", dlerror());
                exit(-2);
            }
            Integral = dlsym(handler2, "SinIntegral1");
            printf("result of integral is %f\n", (*Integral)(atof(argv[2]),atof(argv[3]),atof(argv[4])));
            dlclose(handler2);
        }
    }
}