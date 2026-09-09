#ifndef HANDLER_H
#define HANDLER_H

#include "handlerresult.h"

#define JOB_SUCCESS 0
#define JOB_FAILURE -1

typedef HandlerResult *(*JobHandler)(const void *payload, size_t payload_size);

typedef struct HandlerEntry {
    const char *type;
    JobHandler handler;
} HandlerEntry;

HandlerResult *handler_dispatch(
    const char *type,
    const void *payload,
    size_t payload_size
);

#endif // HANDLER_H