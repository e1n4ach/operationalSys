#include <stdlib.h>
#include <string.h>

char* to_binary(int x) {
    char* result = malloc(33); 
    if (!result) return NULL;

    result[0] = '\0'; 
    int index = 31;
    result[32] = '\0'; 

    for (; index >= 0; index--) {
        result[index] = (x & 1) ? '1' : '0';
        x >>= 1;
    }
    return strdup(result);
}
