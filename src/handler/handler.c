#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "handler.h"
#include "handlerresult.h"
#include "handlers/fibonaccihandler.h"

static const HandlerEntry handler_entries[] = {
    {"fibonacci", fibonacci_handler},
};

HandlerResult *handler_dispatch(
    const char *type,
    const void *payload,
    size_t payload_size
) {
    HandlerResult *result = handler_result_create(
        -1,
        NULL,
        0
    );
    if (type == NULL || payload == NULL) {
        return result;
    }
    for (size_t i = 0; i < sizeof(handler_entries) / sizeof(handler_entries[0]); i++) {
        if (strcmp(handler_entries[i].type, type) == 0) {
            return handler_entries[i].handler(payload, payload_size);
        }
    }

    printf("No handler found for type: %s\n", type);
    return result; // No handler found for the given type
}