#include "argmapper.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
const char *FIBONACCI = "fibonacci";

void *from_arg_to_value(const char *arg, const char* type) {
    if (arg == NULL || type == NULL) {
        return NULL;
    }

    if (strcmp(type, FIBONACCI) == 0) {
        long int *result = malloc(sizeof(long int));
        if (result == NULL) {
            return NULL;
        }
        *result = strtol(arg, NULL, 10);
        return result;
    }

    return NULL;
}