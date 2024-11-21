#include "libderivative.h"
#include <math.h>

double derivative_forward(double x, double deltaX) {
    return (cos(x + deltaX) - cos(x)) / deltaX;
}
