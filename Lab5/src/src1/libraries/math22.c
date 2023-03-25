#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

float derivative2(float A, float deltaX){
    float der = (cos(A+deltaX)-cos(A-deltaX))/(2*deltaX);
    return der;
}