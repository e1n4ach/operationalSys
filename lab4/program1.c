#include "libderivative.h"
#include "libconverter.h"
#include <stdio.h>

int main() {
    double x, deltaX;
    int number;

    // Вычисление производной функции cos(x) в точке x
    printf("Enter value of x for derivative calculation: ");
    scanf("%lf", &x);
    printf("Enter deltaX for derivative calculation: ");
    scanf("%lf", &deltaX);
    
    printf("Derivative (Forward method): %lf\n", derivative_forward(x, deltaX));
    printf("Derivative (Central method): %lf\n", derivative_central(x, deltaX));

    // Перевод числа в систему счисления
    printf("Enter an integer for number system conversion: ");
    scanf("%d", &number);

    printf("Binary representation: %s\n", to_binary(number));
    printf("Ternary representation: %s\n", to_ternary(number));

    return 0;
}
