#include "libderivative.h"
#include <math.h>

double derivative_central(double x, double deltaX) {
    return (cos(x + deltaX) - cos(x - deltaX)) / (2 * deltaX);
}
