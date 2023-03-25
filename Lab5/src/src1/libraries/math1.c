#include "stdio.h"
#include "math.h"

float SinIntegral1(float a, float b, float n){
    int i;
    double h,x,sum=0,integral;
    h=fabs(b-a)/n;
    for(i=1;i<n;i++){
        x=a+i*h;
        sum=sum+sin(x);
    }
    integral=(h/2)*(sin(a)+sin(b)+2*sum);
    return integral;
}
