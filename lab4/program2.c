#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

typedef double (*derivative_func)(double, double);
typedef char* (*converter_func)(int);

int main() {
    void *derivative_lib = NULL;
    void *converter_lib = NULL;

    derivative_func derivative = NULL;
    converter_func to_binary = NULL;
    converter_func to_ternary = NULL;

    int use_forward = 1;

    char command[256];

    printf("\n=================================\n\n");
    printf("Commands:\n");
    printf("0: Switch derivative method\n");
    printf("1 <double> <double>: Calculate Derivative\n");
    printf("2 <int>: Convert number\n");
    printf("3: Show current methods\n");
    printf("4: Exit\n\n");
    printf("=================================\n\n");

    while (1) {
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);

        if (command[0] == '0') {
            use_forward = !use_forward;
            printf("Switched derivative method.\n\n");
        } else if (command[0] == '1') {
            double x, deltaX;
            if (sscanf(command, "1 %lf %lf", &x, &deltaX) != 2) {
                printf("Invalid input. Format: 1 <double> <double>\n\n");
                continue;
            }

            if (use_forward) {
                if (!derivative_lib) {
                    derivative_lib = dlopen("./libderivative.so", RTLD_LAZY);
                }
                if (!derivative) {
                    derivative = dlsym(derivative_lib, "derivative_forward");
                }
            } else {
                if (!derivative_lib) {
                    derivative_lib = dlopen("./libderivative.so", RTLD_LAZY);
                }
                if (!derivative) {
                    derivative = dlsym(derivative_lib, "derivative_central");
                }
            }

            if (!derivative_lib || !derivative) {
                fprintf(stderr, "Error loading derivative library: %s\n", dlerror());
                return 1;
            }

            printf("Derivative: %lf\n\n", derivative(x, deltaX));

        } else if (command[0] == '2') {
            int num;
            if (sscanf(command, "2 %d", &num) != 1) {
                printf("Invalid input. Format: 2 <int>\n\n");
                continue;
            }

            if (!converter_lib) {
                converter_lib = dlopen("./libconverter.so", RTLD_LAZY);
            }

            if (!to_binary) {
                to_binary = dlsym(converter_lib, "to_binary");
            }

            if (!to_ternary) {
                to_ternary = dlsym(converter_lib, "to_ternary");
            }

            if (!converter_lib || !to_binary || !to_ternary) {
                fprintf(stderr, "Error loading converter library: %s\n", dlerror());
                return 1;
            }

            printf("Binary: %s\n", to_binary(num));
            printf("Ternary: %s\n", to_ternary(num));

        } else if (command[0] == '3') {
            printf("Current methods:\n");
            printf("Derivative method: %s\n", use_forward ? "Forward" : "Central");
            printf("\n");

        } else if (command[0] == '4') {
            break;

        } else {
            printf("Unknown command. Valid commands:\n");
            printf("0: Switch derivative method\n");
            printf("1 <double> <double>: Calculate Derivative\n");
            printf("2 <int>: Convert number\n");
            printf("3: Show current methods\n");
            printf("4: Exit\n\n");
        }
    }

    if (derivative_lib) {
        dlclose(derivative_lib);
    }
    if (converter_lib) {
        dlclose(converter_lib);
    }

    return 0;
}
