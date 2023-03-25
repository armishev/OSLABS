#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

float derivative1(float A, float deltaX){
    float der = (cos(A+ deltaX)-cos(A))/deltaX;
    return der;
}
