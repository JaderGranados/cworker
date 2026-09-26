#ifndef ARGMAPPER_H
#define ARGMAPPER_H
#include <stddef.h>

typedef struct MappedValue
{
    void *value;
    size_t size;
} MappedValue;

MappedValue *from_arg_to_value(const char* arg, const char* type);

#endif //ARGMAPPER_H