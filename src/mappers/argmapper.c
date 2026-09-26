#include "argmapper.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
const char *FIBONACCI = "fibonacci";

MappedValue *from_arg_to_value(const char *arg, const char* type) {
    if (arg == NULL || type == NULL) {
        return NULL;
    }

    if (strcmp(type, FIBONACCI) == 0) {
        MappedValue *result = malloc(sizeof(*result));
        if (result == NULL) {
            return NULL;
        }
        long int *mappedArgument = malloc(sizeof(*mappedArgument));
        if (mappedArgument == NULL) {
            free(result);
            return NULL;
        }
        *mappedArgument = strtol(arg, NULL, 10);

        result->value = mappedArgument;
        result->size = sizeof(*mappedArgument);
        return result;
    }

    return NULL;
}