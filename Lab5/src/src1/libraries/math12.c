#include "stdio.h"
#include "math.h"

float SinIntegral2(float a, float b, float n){
    double h,S=0,x;
    int i;
    h=fabs(b-a)/n;
    for(i=0;i<n-1;i++)
    {
        x=a+i*h;
        S=S+sin(x);
    }
    S=h*S;
    return S;
}
