#include <stdlib.h>
#include <string.h>

char* to_ternary(int x) {
    if (x == 0) {
        return strdup("0");
    }

    char buffer[64];
    int index = 63;
    buffer[index] = '\0'; 

    while (x > 0) {
        index--;
        buffer[index] = (x % 3) + '0';
        x /= 3;
    }

    char* result = strdup(&buffer[index]);
    if (!result) return NULL;
    return result;
}
